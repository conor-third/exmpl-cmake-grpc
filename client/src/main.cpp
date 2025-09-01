#include <myproto/address.pb.h>
#include <myproto/addressbook.grpc.pb.h>
#include <myproto/GapGun.pb.h>
#include <myproto/GapGun.grpc.pb.h>

#include <grpc/grpc.h>
#include <grpcpp/create_channel.h>

#include <iostream>


int main(int argc, char* argv[])
{
    // Setup request
    expcmake::NameQuerry query;
    expcmake::Address result;
    query.set_name("John");

    // Call
    auto channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
    std::unique_ptr<expcmake::AddressBook::Stub> stub = expcmake::AddressBook::NewStub(channel);
    grpc::ClientContext context;
    grpc::Status status = stub->GetAddress(&context, query, &result);

    // Output result
    std::cout << "I got:" << std::endl;
    std::cout << "Name: " << result.name() << std::endl;
    std::cout << "City: " << result.city() << std::endl;
    std::cout << "Zip:  " << result.zip() << std::endl;
    std::cout << "Street: " << result.street() << std::endl;
    std::cout << "Country: " << result.country() << std::endl;

    TokenRequest tok;
    TokenRequest tok_res;
    tok.set_token("42");

    std::unique_ptr<GapGunRPCService::Stub> gg_stub = GapGunRPCService::NewStub(channel);
    grpc::ClientContext gg_context;
    status = gg_stub->SetToken(&gg_context, tok, &tok_res);

    std::cout << "Token response: " << tok_res.token() << '\n';

    return 0;
}
