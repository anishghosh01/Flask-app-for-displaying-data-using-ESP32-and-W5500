# Flask-app-for-displaying-data-using-ESP32-and-W5500
Developed a Flask app to display data coming from ESP32 using WebSocket and Ethernet module W5500


Libraries used are: - 
WebSockets2_Generic by Khoi Hoang (v1.13.2)
WebServer_ESP32_W5500 by Gill Maimon, Khoi Hoang (v1.5.3)


Example speed_test.ino is provided which send a sine wave values to the Flask app 


Step1:- To run the python app first enter into the virtual environment using the following command in the terminal after reaching the directory of the file in the terminal: -
.\env\Script\activate.ps1

Step2:- After entering into the virtual environment, use the following command to run the app:-
python .\app.py

After successfully uploading the speed_test.ino into the ESP32, open the local host ip displayed in the terminal (as shown in the terminal of Step 1).

After this you should be able to see the sine wave in the WebApp on your browser.


Tested speeds using Ethernet module W5500 using a Payload of 1024Kb is 540-550kbps.

Using the Sine wave example its about 190-200 kbps
