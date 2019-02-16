FROM peymanmo/cpp-dev-tools:latest

RUN pip3 install cget
RUN cget init --prefix /usr/ --std=c++17
ADD requirements.txt /usr/
RUN cget install --prefix /usr/ -f /usr/requirements.txt

