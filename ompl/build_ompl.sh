    #!/bin/bash

    mkdir -p ompl-1.4.2/build/Release
    cd ompl-1.4.2/build/Release
    cmake ../.. -DPYTHON_EXEC=/usr/bin/python${PYTHONV}
    make
    sudo make install