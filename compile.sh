clear
g++ $(mysql_config --cflags) ClientMain.cpp Client.h Client.cpp Database.h Database.cpp $(mysql_config --libs) -o Client
g++ $(mysql_config --cflags) ServerMain.cpp Server.h Server.cpp Database.h Database.cpp $(mysql_config --libs) -o Server