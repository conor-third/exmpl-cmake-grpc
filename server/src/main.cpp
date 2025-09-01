#include <algorithm>
#include <mutex>
#include <myproto/address.pb.h>
#include <myproto/addressbook.grpc.pb.h>
#include <myproto/addressbook.grpc.pb.h>
#include <myproto/GapGun.grpc.pb.h>
#include <myproto/GapGun.pb.h>

#include <grpc/grpc.h>
#include <grpcpp/server_builder.h>

#include <iostream>

class AddressBookService final : public expcmake::AddressBook::Service {
    public:
        virtual ::grpc::Status GetAddress(::grpc::ServerContext* context, const ::expcmake::NameQuerry* request, ::expcmake::Address* response)
        {
            std::cout << "Server: GetAddress for \"" << request->name() << "\"." << std::endl;

            response->set_name("Peter Peterson");
            response->set_zip("12345");
            response->set_country("Superland");
            
            return grpc::Status::OK;
        }

};

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
            MessageRequest mes_req;
            while(stream->Read(&mes_req))
            {
                std::unique_lock<std::mutex> lock(mu_);
                for(const MessageRequest& mes : received_mes)
                {
                    stream->Write(mes);
                }

                received_mes.push_back(mes_req);
            }
        }

    private:
        std::mutex mu_;
        std::vector<MessageRequest> received_mes;
};

int main(int argc, char* argv[])
{
    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());

    AddressBookService my_service;
    GapGunRPCServiceImpl gg_service;
    builder.RegisterService(&my_service);
    builder.RegisterService(&gg_service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    server->Wait();
    
    return 0;
}
