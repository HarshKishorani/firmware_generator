import json
from zipfile import ZipFile
import gdown
import os
import requests
import datetime
import boto3



def get_topic_name(device_id : str):
    return f"{device_id}/pub"

# https://drive.google.com/file/d/1u_9sbHuP4DCOUqqfd_0iNgwtbNCP7LVW/view?usp=drive_link
POLICY_NAME = "esp_policy"
def create_thing(thing_name, attr, device_name):
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
        return {"certificate" : cert_res["certificatePem"],
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


token = "19824966-f63a-4a6b-a94c-646bdff7ef0c"

save_name = "new_templates_1.zip"

file_id = "1ur_5OzvQX0zXAbD555VJaMdOzrSr8JXn"

def download_templates_file(save_name):
    assert save_name.endswith("zip")
    gdown.download(f'https://drive.google.com/uc?id={file_id}&export=download', save_name, quiet=False)
    with ZipFile(save_name, "r") as zip:
        zip.extractall(f"{save_name[:-4]}")

download_templates_file(save_name)
res = requests.get(base_path + get_product_via_token_endpoint, headers={"token" : token})
print(res.status_code)
if res.status_code == 400:
    print(res.json())
    input("Press any key to exit ...")
    exit()
pinfo = res.json()

device_name = pinfo["base_product_name"]
data_json_path = os.path.join(save_name[:-4], "firmware_generator-main", "templates", device_name,"main", "data.json")
certs_path = os.path.join(save_name[:-4], "firmware_generator-main", "templates", device_name,"main", "certs")
os.makedirs(certs_path, exist_ok=True)
certificate_file = os.path.join(certs_path, "certificate.pem.crt")
private_key_file = os.path.join(certs_path, "private.pem.key")

pinfo["thingName"] = str(pinfo["id"]) + f"_{datetime.datetime.now().strftime('%Y%m%d%H%M%S')}"
attrs = []
for i in pinfo["attributes"]:
    attrs.append(i["attribute_name"])
# print(pinfo)
data = create_thing(pinfo["thingName"], attrs, device_name)

json_data = json.load(open(data_json_path, "r"))
json_data["name"] = pinfo["thingName"]
json.dump(json_data,open(data_json_path,"w"))

with open(private_key_file, "w") as f:
    f.writelines(data["private_key"])

with open(certificate_file, "w") as f:
    f.writelines(data["certificate"])
