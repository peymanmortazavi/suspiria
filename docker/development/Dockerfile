FROM peymanmo/cpp-dev-tools:latest

# Get all C++ dependencies using cGET
ADD requirements.txt /usr
RUN pip3 install cget
RUN cget init --prefix /usr --std=c++17
RUN cget install --prefix /usr -f /usr/requirements.txt --release
