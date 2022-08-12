# Project overview

The objective of this project is the development of an underwater localization and communication system. This system is composed of one
group of three buoys (GoB), placed on the surface of the water, and one or more acoustic units (AU), immersed under the water. A central unit (CU),  
located on the shore or onboard a boat controls it.
    
The architecture of the system is as follows: 

![software%20architecture](https://user-images.githubusercontent.com/60877425/182219691-0d513336-d02f-46c3-8320-1a90ee0f3431.png)


# Hardware requirements

You will need the following equipement :

* For the CU : 1 mobile phone & 1 ESP32 board equipped with LoRa
* For the master buoy : 1 ESP32 board equipped with LoRa, 1 PCB with a Nordic micro-controller, 1 nano-modem & 1 transducer
* For each slave buoy : 1 ESP32 board, 1 PCB with a Nordic micro-controller, 1 nano-modem & 1 transducer


# Software requirements

This project has been entirely coded in C++ on the Arduino IDE. On the IDE, you will need to install the ESP32 board in order to work on it. To do this, follow the tutorial at the following link: https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions. Once done, select the ESP32-WROOM-DA Module board in Tools > Board > ESP32 Arduino.

To compile codes of the project, you will also need to download the following libraries:

* LoRa by Sandeep Mistry - from the IDE library manager
* ThingsBoard by ThingsBoard Team - from the IDE library manager
* SafeString by PowerBroker2 - from GitHub : https://github.com/PowerBroker2/SafeString.git
* PubSubClient by Nick O'Leary - from the IDE library manager
* sMQTTBroker by Vyacheslav Shiryaev - from the IDE library manager
* ArduinoHttpClient by Arduino - from the IDE library manager
* ArduinoJson by Benoît Blanchon - from the IDE library manager

The SPI and WiFi libraries are also required but are normally installed automatically with the installation of the ESP32 board on the IDE.


# How to use the system ?

## Codes

The 4 provided codes correspond to the following hardware :

* CU : ___Coast_board.ino___
* Master buoy : ___Master_buoy.ino___
* Slave buoys 1 & 2 : ___Slave_buoy_1.ino___ & ___Slave_buoy_2.ino___


## Procedure

This section explain the different steps to run the project properly. 

* Step 1 : Download each of the 4 codes on 4 different ESP32 boards.

* Step 2 : Change the IP address of the broker in the ___Master_buoy___, ___Slave_buoy_1___ and ___Slave_buoy_1___ codes, so the MQTT communication can be established. To do so, the value of the ___mqtt_broker___ variable has to be changed with the correct IP address. Since the broker code is run by the master buoy, its IP address corresponds to the master’s IP address. Thus, to retrieve the master’s IP address, open the Serial Monitor on Arduino IDE and look at the line where it is written ___”IP address”___. Then, copy this address and paste it to change the ___mqtt_broker___ variable value in each of the 3 codes.

* Step 3 : On the CU's phone, download an mobile application providing a terminal (_Serial USB Terminal_ or _Bluetooth for Arduino_ for example). Conect it to the _Coast Board_.

* Step 4 : Configure the WiFi AP on which the CU’s ESP32 board will be connected. This WiFi connection is intended to send RNG data from the CU to the Thingsboard dashboard. This step is currently carried out using the serial USB terminal application. From this terminal, after
you have turned on the board, enter the name of the desired WiFi AP when the _"Enter WiFi name :"_ instruction is displayed. Then enter the password when the _"Enter WiFi password :"_ instruction is displayed. A message confirming the connection to the AP and to the ThingsBoard server will appear when it is established. Be careful ! To establish the connection with the server, make sure to activate the WiFi __after__ you have enter the name and password in the terminal. Finally, if you want to change the AP the board is connected to, you will have to reboot the board by pressing its RESET button.

* Step 3 : Put the three buoys' ESP32 boards under power. Please note that the buoys boards must be turned on in a specific order to allow the establishment of the MQTT communication : first the master buoy, then the slave buoy 1 and finally the other slave buoy (slave 2).  Once the MQTT communication is established by one of the buoy, this latter will send a confirmation message, displayed by the CU’s phone, of the form _”NAME BUOY : MQTT
connection established”_, where NAME corresponds to the role of the buoy. his way, if the phone does not receive a confirmation message for each of the 3 buoys, either wait for the buoy(s) to connect or turn the ESP32 boards off and on again in the order indicated above.

* Step 4 : From the terminal application, send the address(es) of the AU(s) to be pinged. If only one AU is to be pinged, the message to be sent should be of the form "$G,NUM,RNG", where NUM corresponds to the three digits identifier of the unit. If several AUs are to be pinged, then the message to be sent should be of the form "$G,NUM1,RNG;$G,NUM2,RNG". Be careful! Do not add any space before or after the semicolon,  at the end of the message or between words, otherwise the ping of some AUs will not be taken into account. For each AU ping, if it went well, the CU will receive six different messages: per buoy, one acknoledgment message (_"$R,TOP,ACK,002,006,AU_NUM"_) and one RNG message (_"$R,NUM,DIST"_), that corresponds to the distance DIST between a buoy
and the unit. 


## Data back-up

The data retrieved by the CU are saved on a web server. To access it, a ThingsBoard dashboard has
been created, which makes it easier to visualise. It is very basic : it only contains a time-series table, displaying the data received in the last 24 hours. To access the dashboard, from a computer, simply go to the following address : https://demo.thingsboard.io/dashboard/5bd46900-069e-11ed-8857-89a1708eda91?publicId=e95a4020-0e72-11ed-9c79-ad222c995ab7.

Be careful ! A trial was used to create this dashboard, so it may not be accessible anymore the moment you read this.

