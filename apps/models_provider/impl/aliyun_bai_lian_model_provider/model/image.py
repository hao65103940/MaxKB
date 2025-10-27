# coding=utf-8
import datetime
from typing import Dict, Optional, Any, Iterator

import requests
from langchain_community.chat_models import ChatTongyi
from langchain_core.language_models import LanguageModelInput
from langchain_core.messages import HumanMessage, BaseMessageChunk, AIMessage
from django.utils.translation import gettext
from langchain_core.runnables import RunnableConfig

from models_provider.base_model_provider import MaxKBBaseModel
from models_provider.impl.base_chat_open_ai import BaseChatOpenAI
import json


class QwenVLChatModel(MaxKBBaseModel, BaseChatOpenAI):

    @staticmethod
    def is_cache_model():
        return False

    @staticmethod
    def new_instance(model_type, model_name, model_credential: Dict[str, object], **model_kwargs):
        optional_params = MaxKBBaseModel.filter_optional_params(model_kwargs)
        chat_tong_yi = QwenVLChatModel(
            model_name=model_name,
            openai_api_key=model_credential.get('api_key'),
            openai_api_base='https://dashscope.aliyuncs.com/compatible-mode/v1',
            # stream_options={"include_usage": True},
            streaming=True,
            stream_usage=True,
            extra_body=optional_params
        )
        return chat_tong_yi

    def check_auth(self, api_key):
        chat = ChatTongyi(api_key=api_key, model_name='qwen-max')
        chat.invoke([HumanMessage([{"type": "text", "text": gettext('Hello')}])])

    def get_upload_policy(self, api_key, model_name):
        """获取文件上传凭证"""
        url = "https://dashscope.aliyuncs.com/api/v1/uploads"
        headers = {
            "Authorization": f"Bearer {api_key}",
            "Content-Type": "application/json"
        }
        params = {
            "action": "getPolicy",
            "model": model_name
        }

        response = requests.get(url, headers=headers, params=params)
        if response.status_code != 200:
            raise Exception(f"Failed to get upload policy: {response.text}")

        return response.json()['data']

    def upload_file_to_oss(self, policy_data, file_stream, file_name):
        """将文件流上传到临时存储OSS"""
        # 构建OSS上传的目标路径
        key = f"{policy_data['upload_dir']}/{file_name}"

        # 构建上传数据
        files = {
            'OSSAccessKeyId': (None, policy_data['oss_access_key_id']),
            'Signature': (None, policy_data['signature']),
            'policy': (None, policy_data['policy']),
            'x-oss-object-acl': (None, policy_data['x_oss_object_acl']),
            'x-oss-forbid-overwrite': (None, policy_data['x_oss_forbid_overwrite']),
            'key': (None, key),
            'success_action_status': (None, '200'),
            'file': (file_name, file_stream)
        }

        # 执行上传请求
        response = requests.post(policy_data['upload_host'], files=files)
        if response.status_code != 200:
            raise Exception(f"Failed to upload file: {response.text}")

        return f"oss://{key}"

    def upload_file_and_get_url(self, file_stream, file_name):
        """上传文件并获取URL"""
        # 1. 获取上传凭证，上传凭证接口有限流，超出限流将导致请求失败
        policy_data = self.get_upload_policy(self.openai_api_key.get_secret_value(), self.model_name)
        # 2. 上传文件到OSS
        oss_url = self.upload_file_to_oss(policy_data, file_stream, file_name)
        print(oss_url)

        return oss_url

    def stream(
            self,
            input: LanguageModelInput,
            config: Optional[RunnableConfig] = None,
            *,
            stop: Optional[list[str]] = None,
            **kwargs: Any,
    ) -> Iterator[BaseMessageChunk]:
        url = "https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions"

        headers = {
            "Authorization": f"Bearer {self.openai_api_key.get_secret_value()}",
            "Content-Type": "application/json",
            "X-DashScope-OssResourceResolve": "enable"
        }

        data = {
            "model": self.model_name,
            "messages": [
                {
                    "role": "user",
                    "content": input[0].content
                }
            ],
            **self.extra_body,
            "stream": True,
        }
        response = requests.post(url, headers=headers, json=data, stream=True)
        if response.status_code != 200:
            raise Exception(f"Failed to get response: {response.text}")
        for line in response.iter_lines():
            if line:
                try:
                    decoded_line = line.decode('utf-8')
                    # 检查是否是有效的SSE数据行
                    if decoded_line.startswith('data: '):
                        # 提取JSON部分
                        json_str = decoded_line[6:]  # 移除 'data: ' 前缀
                        # 检查是否是结束标记
                        if json_str.strip() == '[DONE]':
                            continue

                        # 尝试解析JSON
                        chunk_data = json.loads(json_str)

                        if 'choices' in chunk_data and chunk_data['choices']:
                            delta = chunk_data['choices'][0].get('delta', {})
                            content = delta.get('content', '')
                            if content:
                                yield AIMessage(content=content)
                except json.JSONDecodeError:
                    # 忽略无法解析的行
                    continue
                except Exception as e:
                    # 处理其他可能的异常
                    continue
