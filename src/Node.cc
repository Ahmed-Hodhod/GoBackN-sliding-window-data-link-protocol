#include "Node.h"
#include "Frame_m.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <bitset>
#include <fstream>
#include <string.h>
#include <vector>
#include <cstdio>


Define_Module(Node);
using namespace std;

bool Node::between (int a , int b , int c)
{
    // returns true if b is between a and c circularly
    if ( ( (a <= b ) && (b <= c)) || ( ( c < a) && (b >= a)) || ( (b <= c) && (c < a))   )
    {
        return true;
    }
    return false;
}

void Node::initialize()
{
    next_frame_to_send = 0;
    next_ack_to_send = 0;
    ready_to_send = false; // stop sending after the window
    item = 0; // index of the next location [ Next frame to send =  packets[item] ]
    lower =0; // the lower bound of the sender window
    nbuffered = 0; // number of packets in the window
    acks = vector<int> (par("WS").intValue() + 1, 0 ); // acknowledgement vectors [ for synchronization ]


}


void Node::handleMessage(cMessage *msg)
{


    //redirect the console ouptput to a file
    freopen("output.txt","a",stdout);

    Frame_Base *mmsg = check_and_cast<Frame_Base *>(msg);

   if (mmsg->isSelfMessage()){ // send the next frame and schedule a new one



       //At time [0.5], Node[0] [sent] frame with seq_num=[0] and payload=[ $aaaa$ ] and trailer=[ 0000 ] , Modified [-1 ] , Lost [No], Duplicate [0], Delay [0].
       //At time[2.0 ], Node[1] Sending [ACK] with number [1] , loss [No]

       if ( mmsg->getKind() != 5){
           cancelAndDelete(mmsg);

           cout << "self msg at Time: "<< simTime() << endl;

           Frame_Base *nextFrame = new Frame_Base();
           if (ready_to_send) {
               // get a new packet
               string packet = packets[item];
               string code = codes [ item];
               item +=1;
               nbuffered += 1;

               // frame the packet using byte stuffing
               string frame = "$";
               bitset<8>  parity_byte;
               for (int i = 0 ; i < packet.length() ; i ++ )
               {
                   if (packet[i] == '$' || packet[i] == '/')
                       frame += "/";
                   frame += packet[i];
               }
               frame += "$";

               // calculate the parity byte
               parity_byte =  bitset<8>(frame[0]);
               for ( int i = 1;i < frame.length() ; i ++ ){
                   bitset<8> char_bits (frame[i]);
                   for (int j = 0; j < 8 ; j ++ )
                      parity_byte[j] = parity_byte[j] ^ char_bits[j];
               }

               cout << "At [" << simTime() << "]" << " Introducing channel error with code = [" << code << "]" << endl;

               // Arrival Time at normal operation [ without errors ]
               double ArrivalTime = par("PT").doubleValue() + par("TD").doubleValue();


               nextFrame->setPayload(frame.c_str());
               nextFrame->setSeq_nr(next_frame_to_send);
               nextFrame->setParity_byte(parity_byte.to_string().c_str());
               nextFrame->setKind(0); //kind = 0 for data

               // simulate noisy channel [EROR CODE: Modification,loss,duplication,delay ]
               if (code[1] == '0') // no loss
               {

                   if(code[3] == '1') // delay
                       ArrivalTime += par("ED").doubleValue();


                   if (code[0] == '1') { // modification is introduced to the frame
                       // change the first bit in the first char in the payload
                       bitset<8> char_bits(frame[0]);
                       char_bits[0] = ~ char_bits[0];
                       char modified_char = (char) char_bits.to_ulong();
                       frame[0] = modified_char;
                   }

                    nextFrame->setPayload(frame.c_str());
                    nextFrame->setSeq_nr(next_frame_to_send);
                    nextFrame->setParity_byte(parity_byte.to_string().c_str());
                    nextFrame->setKind(0); //data

                   if (code[2] == '1') // duplication
                   {
                       Frame_Base *dupFrame = new Frame_Base();
                       dupFrame->setPayload(nextFrame->getPayload());
                       dupFrame->setSeq_nr(nextFrame->getSeq_nr());
                       dupFrame->setParity_byte(nextFrame->getParity_byte());
                       dupFrame->setKind(nextFrame->getKind());

                       sendDelayed(dupFrame, ArrivalTime +  par("DD").doubleValue(), "out");
                   }
                    sendDelayed(nextFrame, ArrivalTime , "out");
               }

                cout << "Frame (if not lost) (" << nextFrame->getPayload() << ") with seq_nr ("<< nextFrame->getSeq_nr() <<") and parity byte ("<< nextFrame->getParity_byte()<<") is scheduled to arrive at  " << ArrivalTime + simTime() <<endl;

                next_frame_to_send =  (next_frame_to_send + 1 ) % (1 + par("WS").intValue());

                // simulate a starting timer [timeout mechanism ]
                Frame_Base *timeout_msg = new Frame_Base();
                timeout_msg->setKind(5); //send  a self msg of kind = 5
                timeout_msg->setSeq_nr(next_frame_to_send);
                scheduleAfter(par("TO").doubleValue() + par("PT").doubleValue(), timeout_msg);

                Frame_Base * wakeup = new Frame_Base();
                scheduleAfter(par("PT").doubleValue(), wakeup);

                // check if the window is not exceeded yet
                if( nbuffered < par("WS").intValue()  && item < packets.size()) ready_to_send =true ;
                else ready_to_send = false;

           }
           else // else, not ready
           {
               cout << "Time Now : " << simTime()  << endl;
               cout << "Not ready | item =  " << item << " | nbuffered = " << nbuffered   <<endl;
           }

       }
       else // self msg is in fact a timeout event of some frame
       {

           cout << "Timeout received at time:  " << simTime() << "  On frame: " <<  mmsg->getSeq_nr()   << endl;
           // print acks for debugging
           for (auto item: acks)
               cout  << item << " " ;
           cout << endl;

           if( !acks[mmsg->getSeq_nr()])
           {
            // Go-back N
                // if not sent before because of the timeout [ Finding: Either, it is sent in a previous timeout or the lower was 0000 and this is contradictory ]
                if ( codes[item - nbuffered] != "0000")
                {
                     next_frame_to_send = lower;
                     item -= nbuffered ;
                     mmsg->setKind(-1);
                     scheduleAfter(0.0, mmsg);
                     nbuffered = 0;
                     codes[item ] = "0000";
                     ready_to_send = true;
                }
           }
           else
           {
               acks[mmsg->getSeq_nr()] -= 1;

               // in case the frame is sent before the timeout
               cout << "Ignore the timeout of frame " << mmsg->getSeq_nr() << endl;
           }
       }

     }
    else if(mmsg->getKind() == 3) // message is sent from the hub
    {
        // read the whole data in one shot
       ready_to_send = true;
       fstream newfile;
       if (!strcmp(getName(), "node1"))
           newfile.open("input1.txt",ios::in);
       else
           newfile.open("input0.txt",ios::in);

       if (newfile.is_open()){
          string tp;
          while(getline(newfile, tp)){ //read data from file and put it into string
              packets.push_back( tp.substr(5 ) );
              codes.push_back(tp.substr(0,4));
          }
          newfile.close();
       }
       int start_time =atoi( mmsg->getPayload());
       // initiate the sender to start
       scheduleAfter(start_time, mmsg);

    }
    else if (mmsg->getKind() == 0) // a frame arrived to the Receiver Side
    {

        cout << mmsg->getSeq_nr() << " arrived to the receiver at time :  "<< simTime() << endl;

        Frame_Base * ackFrame = new Frame_Base();

        // check if it is the expected frame
        if (next_ack_to_send == mmsg->getSeq_nr()) // correct frame
        {
            // check the parity
            string rec_frame = mmsg->getPayload();
            bitset<8> check_byte =  bitset<8>(rec_frame[0]);
            for ( int i = 1;i < rec_frame.length() ; i ++ ){
              bitset<8> char_bits (rec_frame[i]);
              for (int j = 0; j < 8 ; j ++ )
                  check_byte[j] = check_byte[j] ^ char_bits[j];
            }

             if ( check_byte.to_string() == mmsg->getParity_byte() )
             {
             next_ack_to_send =  (next_ack_to_send + 1 ) % ( 1 + par("WS").intValue() ) ;
             ackFrame->setSeq_nr(next_ack_to_send);
             ackFrame->setKind(1); // 1 = +ve ack
             cout << mmsg->getSeq_nr() << " is the correct frame.  "<< endl;
             }
             else
             {
                 ackFrame->setKind(2); // 2 = -ve ack (NACK)
                 ackFrame->setSeq_nr(next_ack_to_send);
                 cout << mmsg->getSeq_nr() << " is not correct.  "<< endl;
              }

            }
            else
            {

                ackFrame->setKind(2); // 2 = -ve ack (NACK)
                ackFrame->setSeq_nr(next_ack_to_send);
                cout << mmsg->getSeq_nr() << " is not correct.  "<< endl;
            }

            // consider the loss possibility of ack
            double loss_possibility = par("EL").doubleValue();
            volatile int rand = uniform (0, 1) * 10.0;
            cout << " rand is : " << rand << endl;
            if (rand >= loss_possibility * 10.0 )
            {
               if (ackFrame->getKind() == 1)
                   cout << "ack " << next_ack_to_send <<" sent at time:  " <<  simTime() << "and expected to arrive at " << par("PT").doubleValue() + par("TD").doubleValue() + simTime()<< endl;
               else
                   cout << "NACK " << endl;

               sendDelayed(ackFrame, par("PT").doubleValue() + par("TD").doubleValue(), "out");

            }
        else // else, not the expected frame
        {
            delete ackFrame;
        }

    }
    else if (mmsg->getKind() == 1 or mmsg->getKind() == 2) // if the sender got ack/nack
    {

        if ( mmsg->getKind() == 1)
        {
            cout << "Ack is recived with seq nr: "<< mmsg->getSeq_nr() << "lower before: " << lower  << " at time: "  << simTime()<<endl;

            // implement accumulative ack
            //while (between (lower, mmsg->getSeq_nr(), next_ack_to_send ) )
            if( between (lower, mmsg->getSeq_nr(), next_frame_to_send ))
            {
               nbuffered -- ;
               lower = (lower + 1) % (par("WS").intValue() + 1);
            }
            acks[mmsg->getSeq_nr() ] += 1;
            cout << "After ACK, The New Window:  lower:  " << lower << "Next frame to send: " << next_frame_to_send  << " nbuffered: " << nbuffered<< endl;
            if ( item < packets.size() )
            {
                scheduleAfter(0, mmsg);
                ready_to_send = true;
            }
        }
        else
            cancelAndDelete(mmsg);
        // else, ignore the nack

    }
    else
        cancelAndDelete(mmsg);

    cout << endl;
}
