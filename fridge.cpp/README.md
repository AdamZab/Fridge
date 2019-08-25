Description of program:
Program to monitor two products in the fridge
Program needs 2 variables: brutto weight and netto weight
We have two sensor scales with HX711 connected to arduino
WiFi connection allows to download time from the Internet (NTP) and send PUSH messages to an android phone with the current weight status
For sending messeges PUSH I am using pushingbox.com, for reciving I am using Pushbullet app
According to the assumption, the message will be sent:
- everyday at 16:00
- immediately when a product with 1% -20% content appears on the scale
- after an hour when the weight is below 1% of content or empty (to prevent messages when you are just using the product)

Additionaly OLED display is added, and has functions:
- information about problem with connection
- printing weight status in % and brutto weight of both scales

The program will work with additional scales with no problems, only some information for specific scale must be added. 
OLED will not support more scales for now.
Information need to add for another scale:
- NUMBER_OF_SCALES
- LOADCELL_SDA_PIN
- LOADCELL_SCL_PIN
- SCALE_FACTOR
- netto
- brutto
- SCALE_NAME


Possible improvements:
- read brutto weight and put it in variable (1 and 2)
- enter netto weight and put it in variable (1 and 2)
- tare scale (1 and 2)
- reset arudino
