# Esper Manual


## First Power On
 If Wi-Fi is enabled, the Esper will first have to go through provisioning before it can be used.
 
 When Esper does not have any Wi-Fi credentials stored in memory, it will act as an Wi-Fi access point named "Esper". 
 Connecting to this Wi-Fi and going to http://esper.com will bring up the configuration webpage from which you can
 scan for your Wi-Fi network and attempt to connect to it. 
 
 Once you have successfully connected the IP address that the Esper was given will be shown. After the device has rebooted you will have to assign that IP address as 
 your network's DNS server in your router settings. Unfortunately every router has a different method of configuration, so you will have to search for instructions on how to
 set IP addresses in your router.
 
 It may take a few minutes for your devices to update their DNS server, but after this point Esper will be monitoring DNS requests on your network. 
 To verify that it is working simply go to http://esper.com, and if you are presented with the Esper homepage with a list of recent queries it is working! 
 If not, then something may have gone wrong with the device or configuration
 
 ![homepage](https://i.imgur.com/hzfZoeM.png)
 
 ### Technical Details
When it powers on, Esper will check it's memory for any stored Wi-Fi credentials. If it doesn't find anything it will act as an Wi-Fi access point with a captive portal.
After connecting to it's network, all dns queries will be either blocked, or redirected to the Esper's IP. Some devices may automatically detect this captive portal and 
prompt the user, making it easier to find the configuration website.

## Configuration
Esper ships with a default blacklist of websites so it will start blocking well known ad hosting websites as soon as it is configured as your DNS server. If you still notice
ads coming through, check the recent queries for domain names like "ads.website.com" or "analytics.com" and press the X next to them to add them to the device's blacklist.

### First-Part Ads
Unfortunately, some websites such as Youtube and Hulu serve ads from their own domain (e.g. "youtube.com" and "hulu.com"). Attempting to block these domains will just block some
or the entire website. At the moment there is no way around this other than installing a browser based ad blocker such as uBlock Origin.

## LEDs
The color of the esper displays the current status of the device. Of course, since it is still early in the device's life there are still bugs 
so it may not always be completely accurate.
- Purple - Initializing
- Blue - Blocking
- Weak Blue - Not blocking
- Red - Error

## Button
The button on the side will cause the device to do one of three separate things depending on how long it is held down.
- Less than 0.5 seconds: toggle blocking status
- More than 5 seconds: Wipe current Wi-Fi configuration, go back to provisioning mode
- More than 10 seconds: Revert to previous firmware version 

# FAQ
This is still being developed, but [this post on Reddit](https://old.reddit.com/r/pihole/comments/frum61/frequently_asked_questions/) goes over some common problems setting up
Pi-hole and may be helpful. For all other questions, send an to email esper@openesper.com.
