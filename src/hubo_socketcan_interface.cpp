
#include "HuboCan/SocketCanPump.hpp"
#include "HuboCan/HuboDescription.hpp"
#include "HuboState/State.hpp"
#include "HuboCmd/Aggregator.hpp"
#include "HuboCmd/AuxReceiver.hpp"

using namespace HuboCan;

int main(int argc, char* argv[])
{
    HuboRT::Daemonizer rt;
    if(!rt.begin("socketcan_interface", 49))
    {
        return 1;
    }

    bool virtual_can = false;
    double frequency_override = 0;
    std::string robot_name = "Hubo2Plus";
    for(int i=1; i<argc; ++i)
    {
        if(strcmp(argv[i],"virtual")==0)
        {
            std::cout << "virtual flag noticed -- will run in virtual can mode" << std::endl;
            virtual_can = true;
        }
        else if(strcmp(argv[i],"robot")==0)
        {
            if(i+1 >= argc)
            {
                std::cerr << "The 'robot' argument must be followed by a robot name!" << std::endl;
            }
            else
            {
                robot_name = argv[i+1];
            }
        }
        else if(strcmp(argv[i],"frequency")==0)
        {
            if(i+1 >= argc)
            {
                std::cout << "The 'frequency' argument must be followed by a value!" << std::endl;
            }
            else
            {
                frequency_override = atof(argv[i+1]);
            }
        }
    }

    HuboDescription desc;
    if(!desc.parseFile("/opt/hubo/devices/"+robot_name+".dd"))
    {
        std::cout << "Description could not be correctly parsed! Quitting!" << std::endl;
        return 2;
    }
    desc.broadcastInfo();

    if(frequency_override > 0)
        desc.params.frequency = frequency_override;


    SocketCanPump can(desc.params.frequency, 1e6, desc.params.can_bus_count, 1000, virtual_can);

    can.load_description(desc);

    HuboState::State state(desc);
    HuboCmd::Aggregator agg(desc);
    HuboCmd::AuxReceiver aux(&desc);

    for(size_t i=0; i<desc.jmcs.size(); ++i)
    {
        desc.jmcs[i]->assign_pointers(&agg, &state);
    }

    if(!state.initialized())
    {
        std::cout << "State was not initialized correctly, so we are quitting.\n"
                  << " -- Either your ach channels are not open"
                  << " or your HuboDescription was not valid!\n" << std::endl;
        return 3;
    }

    agg.run();

    while(can.pump() && rt.good())
    {
        state.publish();
        aux.update();
        agg.update();
    }

    return 0;
}
