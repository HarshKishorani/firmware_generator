import json
from zipfile import ZipFile
import gdown
import os
import requests
import datetime
import boto3
import shutil


class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'
def get_topic_name(device_id : str):
    return f"{device_id}/pub"

# https://drive.google.com/file/d/1u_9sbHuP4DCOUqqfd_0iNgwtbNCP7LVW/view?usp=drive_link
POLICY_NAME = "esp_policy"
def create_thing(thing_name, attr, product_name, product_id):
    attr_payload = {
        "attributes" : {}
    }
    
    for i in attr:
        attr_payload["attributes"][i] = "none"
    try:    
        res = client = boto3.client("iot", region_name = "eu-west-1")

        thing_res = client.create_thing(thingName = thing_name,
                                        # attributePayload = attr_payload
        )
        cert_res = client.create_keys_and_certificate(setAsActive=True)
        attach_policy_res = client.attach_policy(
        policyName=POLICY_NAME,
        target = cert_res["certificateArn"]
    )   
        topic_name = get_topic_name(thing_name)
        client.attach_thing_principal(thingName = thing_name, principal = cert_res["certificateArn"])

        res = requests.post(base_path + register_device_api, data={"id" : thing_name, "name" : "none", "product" : product_id})
        if res.status_code == 200:
            print("Device Registered in DB")
        else:
            print("Unable to register device in DB")
        return {
            "certificate" : cert_res["certificatePem"],
            "public_key" : cert_res["keyPair"]["PublicKey"],
            "private_key" : cert_res["keyPair"]["PrivateKey"],
            "topic_name" : topic_name,
            "device_id" : thing_name
            }
                

    except Exception as exc:
        try:
            client.delete_thing(thingName = thing_name)
        except:
            pass
        raise RuntimeError("Unable to craete thing successfully")
        
    

base_path = "http://127.0.0.1:8000/"
get_product_via_token_endpoint = "products/get_product_from_token/"
register_device_api = "v1/api/device_control/device/register"

token = "1da61c35-1d7b-4fbf-b18f-02e655dc77c3"

save_name = "generated_firmware.zip"

file_id = "1ur_5OzvQX0zXAbD555VJaMdOzrSr8JXn"

# print(os.path.exists(r"generated_firmware\firmware_generator-main\templates\switch_device"))

def download_templates_file(save_name):
    assert save_name.endswith("zip")
    gdown.download(f'https://drive.google.com/uc?id={file_id}&export=download', save_name, quiet=False)
    with ZipFile(save_name, "r") as zip:
        zip.extractall(f"{save_name[:-4]}")
    shutil.rmtree("output", ignore_errors=True)
    os.makedirs("output", exist_ok=True)
    # os.path.exists(r"generated_firmware\firmware_generator-main\templates\switch_device")

    shutil.copytree(r"generated_firmware\firmware_generator-main\templates\switch_device", r"output\switch_device") 
    shutil.rmtree(r"generated_firmware")

download_templates_file(save_name)
res = requests.get(base_path + get_product_via_token_endpoint, headers={"token" : token})
print(res.status_code)
if res.status_code == 400:
    print(res.json())
    input("Press any key to exit ...")
    exit()
pinfo = res.json()

device_name = pinfo["base_product_name"]
data_json_path = os.path.join("output", device_name,"main", "data.json")
cmake_path = os.path.join("output", device_name, "CMakeLists.txt") 
certs_path = os.path.join("output", device_name,"main", "certs")
os.makedirs(certs_path, exist_ok=True)
certificate_file = os.path.join(certs_path, "certificate.pem.crt")
private_key_file = os.path.join(certs_path, "private.pem.key")

pinfo["thingName"] = str(pinfo["id"]) + f"_{datetime.datetime.now().strftime('%Y%m%d%H%M%S')}"
attrs = []
for i in pinfo["attributes"]:
    attrs.append(i["attribute_name"])
# print(pinfo)
data = create_thing(pinfo["thingName"], attrs, device_name, pinfo["id"])
print("Device id : "+bcolors.OKGREEN + pinfo["thingName"] + bcolors.ENDC)
json_data = json.load(open(data_json_path, "r"))
json_data["name"] = pinfo["thingName"]
json.dump(json_data,open(data_json_path,"w"))

with open(cmake_path ,"r") as f:
    new = f.read()

new = new.replace("switch_device", pinfo["thingName"])
with open(cmake_path, "w") as f:
    f.write(new)


with open(private_key_file, "w") as f:
    f.writelines(data["private_key"])

with open(certificate_file, "w") as f:
    f.writelines(data["certificate"])

shutil.move("output/switch_device", f"output/{pinfo['thingName']}")