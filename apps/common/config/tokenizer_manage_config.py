# coding=utf-8
"""
    @project: maxkb
    @Author：虎
    @file： tokenizer_manage_config.py
    @date：2024/4/28 10:17
    @desc:
"""
import os
from pathlib import Path

BASE_DIR = Path(__file__).resolve().parent.parent.parent


class MKTokenizer:
    def __init__(self, tokenizer):
        self.tokenizer = tokenizer

    def encode(self, text):
        return self.tokenizer.encode(text).ids


class TokenizerManage:
    tokenizer = None

    @staticmethod
    def get_tokenizer():
        from tokenizers import Tokenizer
        # 创建Tokenizer
        s = os.path.join(BASE_DIR.parent, 'tokenizer', 'bert-base-cased', 'tokenizer.json')
        TokenizerManage.tokenizer = Tokenizer.from_file(s)
        return MKTokenizer(TokenizerManage.tokenizer)
