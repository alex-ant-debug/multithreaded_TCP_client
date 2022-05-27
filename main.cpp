#include "TCP_IP/include/TcpClient.h"

#include <iostream>
#include <stdlib.h>
#include <thread>

#include "../cli-master/include/cli/cli.h"
#include "../cli-master/include/cli/clilocalsession.h"
#include "../cli-master/include/cli/filehistorystorage.h"
#include "../cli-master/include/cli/clifilesession.h"

#include <fstream>

using namespace cli;
using namespace tcp;

// IP 127.0.0.1
uint32_t LOCALHOST_IP = 0x0100007f;

bool runClient(TcpClient& client, uint16_t port) {
  if(client.connectTo(LOCALHOST_IP, port) == SocketStatus::connected) {
    std::string greetings = "Hello, server";
    std::clog << "Client connected\n";
    client.sendData(greetings.c_str(), sizeof(greetings));
    return true;
  } else {
    std::cerr << "Client isn't connected\n";
    std::exit(EXIT_FAILURE);
    return false;
  }
}

int main(int, char**) {

    //ThreadPool thread_pool;

  //TcpClient first_client(&thread_pool);
  //TcpClient second_client(&thread_pool);
  //TcpClient thrird_client(&thread_pool);
  //TcpClient fourth_client(&thread_pool);

  //runClient(first_client);
  //runClient(second_client);
  //runClient(thrird_client);
  //runClient(fourth_client);

  //first_client.joinHandler();
  //second_client.joinHandler();
  //thrird_client.joinHandler();
  //fourth_client.joinHandler();


  // setup cli
  ThreadPool thread_pool;
  //std::vector<TcpClient> many_clients;

  TcpClient first_client(&thread_pool);


  auto rootMenu = std::make_unique< Menu >( "Menu" );
    rootMenu -> Insert(
            "Connect",
            [&](std::ostream& out, uint16_t port) {
                out << "Port: " << port << "\n";
                if(runClient(first_client, port)) {
                    SetColor();
                }
            },
            "Connecting to the server, you must enter the port number" );
    rootMenu -> Insert(
            "Disconnect",
            [&](std::ostream& out) {
                out << "Disconnected from the server!\n";
                first_client.disconnect();
                SetNoColor();
            },
            "Disconnect from the server" );
    rootMenu -> Insert(
            "Number_Clients",
            [](std::ostream& out, uint16_t numberClients) {
                out << "Clients: " << numberClients << "\n";
            },
            "Number of created clients" );


    auto subMenu = std::make_unique< Menu >( "calculator" );
    subMenu -> Insert(
            "Addition",
            [&](std::ostream& out, int16_t a, int16_t b) {
                int32_t result = 0;
                std::string sendingRequest = "add";
                sendingRequest += " " + std::to_string(a) + " " + std::to_string(b);
                first_client.sendData(sendingRequest.c_str(), sizeof(sendingRequest));
                //expect an answer
                out << "Adding two numbers\t" << result << "\n";
            },
            "Adding two numbers" );
    subMenu -> Insert(
            "Subtraction",
            [&](std::ostream& out, int16_t a, int16_t b) {
                int32_t result = 0;
                std::string sendingRequest = "sub";
                sendingRequest += " " + std::to_string(a) + " " + std::to_string(b);
                first_client.sendData(sendingRequest.c_str(), sizeof(sendingRequest));
                //expect an answer
                out << "Subtraction two numbers\t" << result << "\n";
            },
            "Subtraction of two numbers" );
    subMenu -> Insert(
            "Multiplication",
            [&](std::ostream& out, int16_t a, int16_t b) {
                int32_t result = 0;
                std::string sendingRequest = "mul";
                sendingRequest += " " + std::to_string(a) + " " + std::to_string(b);
                first_client.sendData(sendingRequest.c_str(), sizeof(sendingRequest));
                //expect an answer
                out << "Multiplication two numbers\t" << result << "\n";
            },
            "Multiplication of two numbers" );
    subMenu -> Insert(
            "Division",
            [&](std::ostream& out, int16_t a, int16_t b) {
                int32_t result = 0;
                std::string sendingRequest = "div";
                sendingRequest += " " + std::to_string(a) + " " + std::to_string(b);
                first_client.sendData(sendingRequest.c_str(), sizeof(sendingRequest));
                //expect an answer
                out << "Division two numbers\t" << result << "\n";
            },
            "Division of two numbers" );


    auto subSubMenu = std::make_unique< Menu >( "subsub" );
        subSubMenu -> Insert(
            "hello",
            [](std::ostream& out){ out << "Hello, subsubmenu world\n"; },
            "Print hello world in the sub-submenu" );

    subMenu -> Insert( std::move(subSubMenu));

    rootMenu -> Insert( std::move(subMenu) );

    Cli cli( std::move(rootMenu) );
    // global exit action
    cli.ExitAction( [&](auto& out){
        out << "Goodbye and thanks for all the fish.\n";
        std::exit(0);
    } );

    CliFileSession input(cli);
    input.Start();

    first_client.joinHandler();

  return 0;
}
