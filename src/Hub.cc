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

#include "Hub.h"
#include "Frame_m.h"

Define_Module(Hub);
using namespace std;

#include <filesystem>
#include <iostream>
#include <unistd.h>
#include <fstream>


void Hub::initialize()
{

   // what is the msg content
   Frame_Base * msg = new Frame_Base();
   msg->setKind(3); //msg from hub


   char node;
   string starting_time;

   fstream newfile;
   newfile.open("coordinator.txt",ios::in); //open a file to perform read operation using file object
   if (newfile.is_open()){
      string tp;
      while(getline(newfile, tp)){ //read data from file object and put it into string.
          node = tp[0];
          starting_time = tp.substr(2);
      }
      newfile.close();
   }
   msg->setPayload(starting_time.c_str());

   if (node == '0')
       send(msg, "out0");
   else
       send (msg, "out1");

}

void Hub::handleMessage(cMessage *msg)
{
    // TODO - Generated method body
}
