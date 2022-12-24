//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef __FALL22_PROJECT_NODE_H_
#define __FALL22_PROJECT_NODE_H_

#include <omnetpp.h>
#include <string.h>
#include <vector>
#include "Frame_m.h"

using namespace omnetpp;

/**
 * TODO - Generated class
 */
class Node : public cSimpleModule
{
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    bool between (int a , int b , int c);

  private:

    int next_frame_to_send;
    int next_ack_to_send;
    int expected_frame_to_receive;
    bool ready_to_send;
    std::vector<Frame_Base*> buffer;
    int nbuffered;
    int item ;


    std::vector<int> acks;

    int lower; // = ack_expected, not yet acknowledged
    int upper;
    std::vector<std::string> packets;
    std::vector<std::string> codes;
};

#endif
