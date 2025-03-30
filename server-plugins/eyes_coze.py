from plugins_func.register import register_function, ToolType, ActionResponse, Action
from cozepy import Coze, TokenAuth, Message, ChatEventType, MessageObjectString  # noqa
import requests  
eyes_coze_function_desc = {
    "type": "function",
    "function": {
        "name": "eyes_coze",
        "description": "小智的眼睛, 可以识别小智眼前的东西",
        'parameters': {'type': 'object', 'properties': {}, 'required': []}
    }
}



@register_function('eyes_coze', eyes_coze_function_desc, ToolType.WAIT)
def eyes_coze():
    url = 'http://你的开发板ip/jpg'  
    response = requests.get(url, timeout=2000)
    response = requests.get(url, timeout=2000)

    if response.status_code == 200:  
        with open('tmp/image.jpg', 'wb') as file:  
            file.write(response.content)  
            print('文件下载成功')  
    else:  
        print('文件下载失败')  
    from cozepy import COZE_CN_BASE_URL

    # Get an access_token through personal access token or oauth.
    coze_api_token = "你的token"
    # The default access is api.coze.com, but if you need to access api.coze.cn,
    # please use base_url to configure the api endpoint to access
    coze_api_base = COZE_CN_BASE_URL

    # Init the Coze client through the access_token.
    coze = Coze(auth=TokenAuth(token=coze_api_token), base_url=coze_api_base)

    # Create a bot instance in Coze, copy the last number from the web link as the bot's ID.
    bot_id = "你的机器人id"
    # The user id identifies the identity of a user. Developers can use a custom business ID
    # or a random string.
    user_id = "abc123"

    from pathlib import Path  # noqa

    # Call the upload interface to upload a picture requiring text recognition, and
    # obtain the file_id of the picture.
    file_path = "tmp/image.jpg" # 上传图片
    file = coze.files.upload(file=Path(file_path))

    # Call the coze.chat.stream method to create a chat. The create method is a streaming
    # chat and will return a Chat Iterator. Developers should iterate the iterator to get
    # chat event and handle them.
    response_text = ""
    for event in coze.chat.stream(
        bot_id=bot_id,
        user_id=user_id,
        additional_messages=[
            Message.build_user_question_objects(
                [
                    MessageObjectString.build_text("这个图片是关于什么的?"),
                    MessageObjectString.build_image(file_id=file.id),
                ]
            ),
        ],
    ):
        if event.event == ChatEventType.CONVERSATION_MESSAGE_DELTA:
            message = event.message
            print(event.message.content, end="", flush=True)
            if message.content:
                response_text += message.content
    
    return ActionResponse(Action.RESPONSE, None, response_text)