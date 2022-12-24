package fall22_project;

import ned.IdealChannel;


@license(LGPL);
//
// TODO documentation
//
network Network
{
    @display("bgb=627,397");
    submodules:
        hub: Hub {
            @display("p=278,199");
        }
        node0: Node {
            @display("p=108,72");
        }
        node1: Node {
            @display("p=382,102");
        }
    connections:
        hub.out1 --> IdealChannel --> node1.in0;
        hub.out0 --> IdealChannel --> node0.in0;
        node1.out --> IdealChannel --> node0.in1;
        node0.out --> IdealChannel --> node1.in1;
}

