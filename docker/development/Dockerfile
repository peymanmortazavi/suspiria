FROM peymanmo/cpp-dev-tools:latest

# Get all C++ dependencies using cGET
ADD requirements.txt /usr/lib
RUN pip3 install cget
RUN cget init --prefix /usr --std=c++17
RUN cget install --prefix /usr -f /usr/lib/requirements.txt
