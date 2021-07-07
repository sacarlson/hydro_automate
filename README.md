# hydro_automate
Arduino system to automate a hydroponic fluid system to remote monitor and auto feedback of TDS readings and fluid levels. 

This is a total integration of fluid pumps for grow solutions A and B, TDS and other measurment devices with an Arduino esp8266 using Blynk libs.  The android blynk app is used for user feedback and control of the system.  It also integrates fluid level switches to monitor the fluid levels in the hydroponic reservoir.  This integrates remote monitoring using your android or IOS phone to view history and present status of TDS and other readings over time.  This package also records and presents the doses of the hydro solutions that had been added to the reservoir over time.  the system can be set to be controled manually or with full auto feedback control with set values of TDS level trigers from the Blynk app user interface.


In this project I selected to use a private blynk server that is also open source on a vps system I have.  I am not sure how well it will work on the blynk cloud server service but give it a try and tell me if you do.

This uses the older open source version of Blynk (legacy) libs and the (legacy) Blynk app that is available on google play store at: https://play.google.com/store/apps/details?id=cc.blynk&hl=en&gl=US

To install install the hydro_automate into blynk see the provided barcode image that will install this user interface into your blynk app.

You will have to modify the token line in  the *.ino file to your token number that can be found in your blynk app settings.
you will also have to modify the lines that include the wifi access point name and password to match your resedence settings.

If you have any questions you can leave an issue here on github or try contact me on https://chat.funtracker.site/channel/hydro_automate

Note: This project is also an example of how you can integrate OTA (Over The Air) firmware updates with just an esp8266 and the opensource version of Blynk.  To do this you do still have to be residing on the same wifi network to flash changes, not just anyplace on the internet like I think you can on the new version of Blynk.
