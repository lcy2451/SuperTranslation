# 这是一个示例 Python 脚本。

# 按 Shift+F10 执行或将其替换为您的代码。
# 按 双击 Shift 在所有地方搜索类、文件、工具窗口、操作和设置。
import os
from pathlib import Path


import json
from openai import OpenAI

class DeepSeekProvider:

    @classmethod
    def deep_seek_api(cls):
        deepseek_api_json: Path = Path(os.environ["SUPER_TRANSLATION_TEMP"]) / "DeepSeekApiKey.txt"
        with open(deepseek_api_json, "r", encoding="utf-8") as f:
            api_key = f.read()

        return api_key


    @classmethod
    def test(cls):
        temp_json:Path = Path(os.environ["SUPER_TRANSLATION_TEMP"]) / 'DeepSeek.json'
        print(f'temp_dir {temp_json} {temp_json.exists()}')

        result = cls.translate('Static Mesh Component')
        print(result)
        with open(temp_json, "w", encoding="utf-8") as f:
            json.dump(result, f, ensure_ascii=False, indent=4)

    @classmethod
    def translate_and_save(cls, text: str, target_lang:str='zh', source_lang: str='en') -> dict:
        """
        新的通用函数：传入待翻译文本，自动调用 API 并保存至中间 JSON 文件夹
        :param text: 待翻译的英文字符串
        :param file_name: 写入的文件名，默认 DeepSeek.json
        :return: 翻译结果字典
        """
        temp_dir = os.environ.get("SUPER_TRANSLATION_TEMP")
        if not temp_dir:
            print("ERROR: 环境变量 SUPER_TRANSLATION_TEMP 未设置")
            return {}

        temp_json: Path = Path(temp_dir) / "DeepSeek.json"

        # 1. 调用 API 获取原始结果
        result = cls.translate(text, target_lang=target_lang, source_lang=source_lang)

        # 2. 为了防止 UE C++ 解析 Array 崩溃，生成一个纯 FString:FString 兼容的平铺字典
        # 如果逻辑需要，可以在 C++ 读取的字段里把 alternatives 拼成纯字符串
        flat_result = {
            "source": str(result.get("source", "")),
            "translation": str(result.get("translation", "")),
            "alternatives": result.get("alternatives", '')  # 把数组拼成 "选项1, 选项2" 纯字符串
        }

        print(f"[DeepSeekProvider] 翻译完成 ({text}) -> 保存至: {temp_json}")

        # 3. 写入文件
        with open(temp_json, "w", encoding="utf-8") as f:
            json.dump(flat_result, f, ensure_ascii=False, indent=4)

        return flat_result

    @classmethod
    def translate(cls, text:str, target_lang:str='zh', source_lang: str='en') -> dict:
        if target_lang == 'zh':
            target_lang = 'Simplified Chinese'
        elif target_lang == 'en':
            target_lang = 'English'


        if source_lang == 'zh':
            source_lang = 'Simplified Chinese'
        elif source_lang == 'en':
            source_lang = 'English'

        system_prompt = f"""
        You are a professional Unreal Engine technical translator.

        Translate Unreal Engine related {source_lang} text into {target_lang}.

        Rules:
        1. Use official Unreal Engine {target_lang} terminology when available.
        2. Keep Unreal class names and API names unchanged when appropriate.
        3. Provide the best translation.
        4. Provide alternative translations.
        5. Return only valid JSON.
        """
        system_prompt += """
        JSON format:
        {
            "source": "",
            "translation": "",
            "alternatives": []
        }
        """

        with OpenAI(
                api_key=cls.deep_seek_api(),
                base_url="https://api.deepseek.com",
        ) as client:
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
