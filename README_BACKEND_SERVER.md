On mysys2 ucrt64 terminal 


$ cd /c/Users/fuent/Downloads/senior/backend2

$ g++ main.cpp Logger.cpp SCPI_Driver.cpp -o main.exe -lws2_32

$ ./main.exe

(server listening)
(receive call)



On CMD prompt


curl -X POST http://localhost:8080/scpi -d ":ROUTE:PATH 3" // calls path 3
Output:
{"address":268436224,"component_id":"switch1","gpio_value":3,"id":3}
Test another path
 curl -X POST http://localhost:8080/scpi -d ":ROUTE:PATH 4" //path 4
If invalid output: 
 curl -X POST http://localhost:8080/scpi -d ":ROUTE:PATH 5"
Output:
{"error":"Path ID not found"}
Paste into cmd 
curl -X POST http://localhost:8080/scpi -d ":ROUTE:PATH 2"
output
{"address":268436224,"component_id":"switch1","gpio_value":2,"id":2}
