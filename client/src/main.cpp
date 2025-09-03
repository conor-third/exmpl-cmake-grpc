#include <grpcpp/support/sync_stream.h>
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
    std::unique_ptr<GapGunRPCService::Stub> gg_stub = GapGunRPCService::NewStub(channel);
    grpc::ClientContext context;

    std::shared_ptr<::grpc::ClientReaderWriter<MessageRequest, MessageRequest>> stream = gg_stub->SubscribeToMessages(&context);

    std::thread reader([stream]()
    {
        MessageRequest mes_req;
        while(stream->Read(&mes_req))
        {
            std::cout << "Received server msg: id = " << mes_req.request_id() << " json = " << mes_req.json() << '\n';

            MessageRequest reply;
            reply.set_request_id(mes_req.request_id());
            reply.set_json("Response from client");

            if(!stream->Write(reply))
                break;

            std::cout << "[Client] Sent response for id=" << reply.request_id() << std::endl;
        }

        std::cout << "[Client] Reader thread exiting" << std::endl;
    });

    reader.join();

    stream->WritesDone();
    stream->Finish();
    std::cout << "[Client] Stream finished" << std::endl;

    return 0;
}
