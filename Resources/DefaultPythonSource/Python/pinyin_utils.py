import os
import re
from pathlib import Path

from pinyintokenizer import PinyinTokenizer
from Pinyin2Hanzi import DefaultDagParams, dag
from wordfreq import zipf_frequency


tokenizer = PinyinTokenizer()
dag_params = DefaultDagParams()


def split_camel_case(orign_text: str) -> list[str]:
    return re.findall(
        r"[A-Z]+(?=[A-Z][a-z]|\d|$)|[A-Z]?[a-z]+|\d+",
        orign_text
    )


# 仅支持通过 _、数字或大小写边界分隔的拼音/英文混合命名。
# 不支持无分隔、全小写的拼音与英文混写，例如 jinshumetal。
def pinyin_to_chinese(orign_text: str, candidate_count: int = 5) -> str:

    result = ''
    for text in orign_text.split("_"):

        # 全大写的话视为英文
        if text == text.upper():
            # print(f'{text} 全大写了')
            result += f'{text}_'
            continue

        pinyin_to_check_list = []
        # letters_only = re.sub(r"\d+", "", text)
        words = split_camel_case(text)

        _result = ''
        for word in words:
            score = zipf_frequency(word.lower(), "en")
            if score < 1:
                pinyin_to_check_list.append([word, True])
            else:
                pinyin_to_check_list.append([word, False])

        for pinyin_to_check in pinyin_to_check_list:

            if pinyin_to_check[1]:
                pinyin_list, invalid_parts = tokenizer.tokenize(pinyin_to_check[0].lower())

                if invalid_parts:
                    _result += pinyin_to_check[0]
                    continue
                _r =  dag(
                    dag_params,
                    pinyin_list,
                    path_num=candidate_count,
                    log=True,
                )
                if _r:
                    _result += _r[0].path[0]
                else:
                    _result += pinyin_to_check[0]
            else:
                _result += pinyin_to_check[0]
            # print(f"{pinyin_to_check} result {result}")
        result += f'{_result}_'

    result = result[:-1]
    return result

def pinyin_check_save(orign_text: str):
    temp_dir = os.environ.get("SUPER_TRANSLATION_TEMP")

    if not temp_dir:
        print("ERROR: 环境变量 SUPER_TRANSLATION_TEMP 未设置")
        return

    temp_file: Path = Path(temp_dir) / "has_pinyin"
    _r = pinyin_to_chinese(orign_text)
    with open(temp_file, "w", encoding="utf-8") as f:
        f.write(_r)

if __name__ == '__main__':
    # for text in ["dagoujiaojiaoMa", "xiaomao", "BP_heisewenli", "jinshucaizhi", "PHYS_New"]:
    # for text in ["BP_heisewenli"]:
    # for text in ["PHYS_New1", "BP_dagou_New1", "MP_jinshuMetal"]:
    # for text in ["MP_jinshuMetal", "dagoujiaojiaoMa", "xiaomao", "BP_heisewenli", "jinshucaizhi", "PHYS_New111"]:
    for text in ["BP_NewBlueprint17"]:
        print(text, pinyin_to_chinese(text))
        # print(text, "->", pinyin_to_hanzi(text))
