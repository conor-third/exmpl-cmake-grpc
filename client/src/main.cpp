#include <memory>
#include <thread>
#include <myproto/GapGun.pb.h>
#include <myproto/GapGun.grpc.pb.h>

#include <grpc/grpc.h>
#include <grpcpp/create_channel.h>

#include <iostream>
#include <vector>

MessageRequest MakeMessageRequest(const std::string& id, const MessageType type, const std::string& json)
{
    MessageRequest req;
    req.set_request_id(id);
    req.set_type(type);
    req.set_json(json);
    return req;
}

int main(int argc, char* argv[])
{
    // Call
    auto channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());

    TokenRequest tok;
    TokenRequest tok_res;
    tok.set_token("42");

    std::unique_ptr<GapGunRPCService::Stub> gg_stub = GapGunRPCService::NewStub(channel);
    grpc::ClientContext gg_context;
    ::grpc::Status status = gg_stub->SetToken(&gg_context, tok, &tok_res);
    std::cout << "Token response: " << tok_res.token() << '\n';
    
    grpc::ClientContext gg_context_bi;
    std::unique_ptr<GapGunRPCService::Stub> gg_stub_bi = GapGunRPCService::NewStub(channel);
    std::shared_ptr<::grpc::ClientReaderWriter<MessageRequest, MessageRequest>> stream(gg_stub_bi->SubscribeToMessages(&gg_context_bi));

    std::thread writer([stream](){
            std::vector<MessageRequest> reqs {MakeMessageRequest("42", MessageType::HealthCheck, "CONOR")};
            for(const MessageRequest& req : reqs)
            {
                std::cout << "Sending message - id: " << req.request_id() << '\n';
                stream->Write(req);
            }
            stream->WritesDone();
        });

    MessageRequest server_mes_req;
    while(stream->Read(&server_mes_req))
    {
        std::cout << "Got message: request_id: " << server_mes_req.request_id() << '\n';
    }
    writer.join();
    status = stream->Finish();
    if(!status.ok())
        std::cout << "SubscribeToMessages failed!\n";

    return 0;
}
