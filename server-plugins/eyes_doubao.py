from plugins_func.register import register_function, ToolType, ActionResponse, Action
import requests  
from volcenginesdkarkruntime import Ark
import base64
eyes_function_desc = {
    "type": "function",
    "function": {
        "name": "eyes_doubao",
        "description": "小智的眼睛, 可以识别小智眼前的东西",
        'parameters': {'type': 'object', 'properties': {}, 'required': []}
    }
}

#图片转base64函数
def encode_image(image_path):
    with open(image_path, "rb") as image_file:
        return base64.b64encode(image_file.read()).decode('utf-8')

@register_function('eyes_doubao', eyes_function_desc, ToolType.WAIT)
def eyes_doubao():
    url = 'http://192.168.248.171/jpg'  
    response = requests.get(url, timeout=2000)
    response = requests.get(url, timeout=2000)

    if response.status_code == 200:  
        with open('tmp/image.jpg', 'wb') as file:  
            file.write(response.content)  
            print('文件下载成功')  
    else:  
        print('文件下载失败')  

    # 请确保您已将 API Key 存储在环境变量 ARK_API_KEY 中
    # 初始化Ark客户端，从环境变量中读取您的API Key
    client = Ark(
        # 此为默认路径，您可根据业务所在地域进行配置
        base_url="https://ark.cn-beijing.volces.com/api/v3",
        # 从环境变量中获取您的 API Key。此为默认方式，您可根据需要进行修改
        api_key="9b8d8fc9-402b-495b-a0a3-552670340f33",
    )

    #输入图片路径
    image_path = "tmp/image.jpg"
    
    #原图片转base64
    base64_image = encode_image(image_path)

    response = client.chat.completions.create(
        # 指定您创建的方舟推理接入点 ID，此处已帮您修改为您的推理接入点 ID
        model="doubao-1-5-vision-pro-32k-250115",
        messages=[
            {
                "role": "user",
                "content": [
                    {"type": "text", "text": "描述一下这个图片"},
                    {
                        "type": "image_url",
                        "image_url": {
                            "url": f"data:image/jpeg;base64,{base64_image}"
                        },
                    },
                ],
            }
        ],
        
    )

    print(response.choices[0].message.content)
    
    return ActionResponse(Action.RESPONSE, None, response.choices[0].message.content)