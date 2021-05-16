# Esper Manual

## Setting up Esper
In order for Esper to block ads, your devices have to be told to use it as their DNS server. You can do this individually for each device by going into their internet/wifi settings and manually changing the assigned DNS server, or you can do it for every device on your network at once by editing the DNS settings in your router. Unfortunately every router has a different method of configuration, so you will have to search for instructions online on how to set IP addresses in your router.

It may take a few minutes for your devices to update their DNS server, but after this point Esper will be monitoring DNS requests on your network. 
To verify that it is working simply go to http://esper.com (not "https://" !!), and if you are presented with the Esper homepage it is working! 
If not, then something may have gone wrong with the device or DNS server configuration

## Finding Esper's IP

Similar to before, you will have to go into your router's settings and look for Esper's IP there. Most routers have a "connected devices" page that can be helpful. 

## Configuration
Esper ships with a default blacklist of websites so it will start blocking well known ad hosting websites as soon as it is configured as your DNS server. If you still notice
ads coming through, check the recent queries for domain names like "ads.website.com" or "analytics.com" and press the 'X' next to them to add them to the device's blacklist.

### First-Part Ads
Unfortunately, some websites such as Youtube and Hulu serve ads from their own domain (e.g. "youtube.com" and "hulu.com"). Attempting to block these domains will just block some
or the entire website. At the moment there is no way around this other than installing a browser based ad blocker such as uBlock Origin.

## LEDs
The color of the esper displays the current status of the device.
- Purple - Initializing
- Blue - Blocking
- Weak Blue - Not blocking
- Red - Error

## Button
The button on the side will cause the device to do one of two things depending on how long it is held down.
- Less than 0.5 seconds: toggle blocking status
- More than 8 seconds: Factory reset

# FAQ
This is still being developed, but [this post on Reddit](https://old.reddit.com/r/pihole/comments/frum61/frequently_asked_questions/) goes over some common problems setting up
Pi-hole and may be helpful if you are having a similar problem with Esper. For all other questions, send an to email esper@openesper.com.
