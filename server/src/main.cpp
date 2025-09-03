#include <algorithm>
#include <mutex>
#include <myproto/GapGun.grpc.pb.h>
#include <myproto/GapGun.pb.h>

#include <grpc/grpc.h>
#include <grpcpp/server_builder.h>

#include <iostream>
#include <thread>
#include <chrono>

class GapGunRPCServiceImpl final : public GapGunRPCService::Service
{
        virtual ::grpc::Status SetToken(::grpc::ServerContext* context, const ::TokenRequest* token_request, ::TokenRequest* token_response) override
        {
            std::cout << "setting token\n";
            token_response->set_token(token_request->token());
            token_response->set_token("43");
            return ::grpc::Status::OK;
        }

        virtual ::grpc::Status SubscribeToMessages(::grpc::ServerContext* context, ::grpc::ServerReaderWriter<MessageRequest, MessageRequest>* stream) override
        {
            std::thread writer([stream, context]()
            {
                int counter = 0;
                while(!context->IsCancelled())
                {
                    MessageRequest mes_req;
                    mes_req.set_request_id(std::to_string(counter++));
                    mes_req.set_type(MessageType::HealthCheck);
                    mes_req.set_json("ping from server");

                    if(!stream->Write(mes_req))
                        break;

                    std::cout << "Server sent message id = " << mes_req.request_id() << '\n';
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                }
                std::cout << "Writer thread exiting....\n";
            });

            MessageRequest client_res;
            while(stream->Read(&client_res))
            {
                std::cout << "Received response id = " << client_res.request_id()
                    << " json: " << client_res.json() << '\n';
            }

            writer.join();
            std::cout << "stream closed\n";
            return ::grpc::Status::OK;
        }

    private:
        std::mutex mu_;
        std::vector<MessageRequest> received_mes;
};

int main(int argc, char* argv[])
{
    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());

    GapGunRPCServiceImpl gg_service;
    builder.RegisterService(&gg_service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    server->Wait();
    
    return 0;
}
