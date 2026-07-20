# 这是一个示例 Python 脚本。

# 按 Shift+F10 执行或将其替换为您的代码。
# 按 双击 Shift 在所有地方搜索类、文件、工具窗口、操作和设置。
import os
from pathlib import Path
os.environ["DEEPSEEK_API_KEY"] = 'sk-5bf6f0e127d84ba8b20579fd025a0e38'


import json
from openai import OpenAI

client = OpenAI(
    api_key='sk-5bf6f0e127d84ba8b20579fd025a0e38',
    base_url="https://api.deepseek.com",
)

class DeepSeekProvider:

    @classmethod
    def test(cls):
        temp_json:Path = Path(os.environ["SUPER_TRANSLATION_TEMP"]) / 'DeepSeek.json'
        print(f'temp_dir {temp_json} {temp_json.exists()}')

        result = cls.translate('Static Mesh Component')
        print(result)
        # with open(str(temp_json)) as f:
        #     json.dump(result, f)


    @classmethod
    def translate(cls, text:str) -> dict:
        system_prompt = """
        You are a professional Unreal Engine technical translator.

        Translate Unreal Engine related English text into Simplified Chinese.

        Rules:
        1. Use official Unreal Engine Chinese terminology when available.
        2. Keep Unreal class names and API names unchanged when appropriate.
        3. Provide the best translation.
        4. Provide alternative translations.
        5. Return only valid JSON.

        JSON format:
        {
            "source": "",
            "translation": "",
            "alternatives": []
        }
        """

        response = client.chat.completions.create(
            model="deepseek-chat",
            messages =[
                {"role": "system", "content": system_prompt},
                {"role": "user","content": text}
            ],
            response_format = {
                "type": "json_object"
            }
        )

        result = json.loads(
            response.choices[0].message.content
        )

        return result

    @staticmethod
    def rename_actor():
        pass
