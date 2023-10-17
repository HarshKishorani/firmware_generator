# RGB lighting Firmware

To generate firmware for your product perform the following steps :

1. Copy this project to your desired destination.
2. Create a thing with **unique thing name**(this will be your device id) in AWS Iot Core Platform and get certificates.
3. Edit the certificates at path : `main\certs`.
4. Get developer's product configration using the ***token*** provided and edit the `main\data.json` file with developer's product configration.
5. Now you have successfully created a new firmware for developer's device with **device_id = unique thing name you gave on AWS Iot** for developer's product with unique product_id.

## Parametes
| Parameter Name | Data Type | Range |
| --------------- | --------------- | --------------- |
| power | bool | - |
| colors_text | string | - |
| mode | int | 1-3 |
| speed | int | 1-100 |
| reboot | bool | - |
| wifi_reset | bool | - |
| factory_reset | bool | - |

## Working of Cloud

- #### Device to Cloud updates
  
  1. When state of any attribute is changed manually from device, device will publish change updates to `device_id/pub`.
  2. Eg. of change update
    ```
    {
        "mode" : 3
    }
  ```
  3. The change update will then be routed to our Backend Server from AWS Iot Core; and then updates will be updated in our Database.

    Note : Device will publish one update at a time. 

- #### Cloud to Device updates
  1. When user wants to change state of any attribute of device, it sends **http request** from our mobile app to our Backend Server.
  2. Then our Backend Server will publish update to `device_id/sub` (to which device is listening). Do not save updates in Database yet.
  3. The device will then receive updates and perform changes and then send updates back using **Device to Cloud update function**. Then save changes to Database as discussed in 3rd point of **Device to Cloud updates**. Let's call this ***Save on receive update.***

  Note:  Publish one change update at a time to device as shown on left side and not as shown on right side.

<table>
<tr>
<td style="border-right: 1px solid #000; padding-right: 10px;">
<pre><code>{
    "mode" : 3   
}</code></pre>
</td>
<td style="padding-left: 10px;">
<pre><code>{
    "mode" : 3
    "colors_text" : "FF000000FF000000FF",      
}</code></pre>
            </td>
        </tr>
        </table>

## Pending tasks
 
- [x] Boot Interrupt
- [ ] Function to get Subscribe and Publish topics.
- [x] Cloud to Device Updates (Subscribe to updates from cloud).
- [x] Device to Cloud Updates (Publish updates from cloud).
- [x] Factory Reset, Wifi Reset and Reboot Device functions.
- [ ] Function to get all values from cloud once device boots / starts. 
- [ ] Update online status to cloud on system boot and system power off.
- [ ] Add resetting flag in endless while loop (Check rgb_esp_idf repo).
- [ ] Wifi App event Handler