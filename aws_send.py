import sys
import aws_config as config   
import json
sys.path.append("./libboto3")
import boto3

client = boto3.client('iot-data', aws_access_key_id = config.aws_access_key_id, aws_secret_access_key = config.aws_secret_access_key , region_name= config.region, endpoint_url=config.endpoint)

# sending json file to aws iot
message = {}
message['Device'] = "qcs610"
message['time'] = sys.argv[1][:-1]
message['Total_count'] = sys.argv[2]
message['People_in_ROI'] = sys.argv[3]
message_json = json.dumps(message)

response = client.publish(
                     topic = config.topicname,
                     qos = 1,
                     payload = message_json
                     )
                     
if response['ResponseMetadata']['HTTPStatusCode'] == 200:
    print("Info send to AWS")
