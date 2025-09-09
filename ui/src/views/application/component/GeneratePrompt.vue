<template>
  <el-dialog align-center :title="$t('生成提示词')" v-model="dialogVisible"
    style="width: 600px">

    <div>
      <div class="dialog-bg">
        <div class="scrollbar-height">
          <!-- 生成内容 -->
          <div style="height: 600px;">
            <el-card shadow="always" class="mb-8 border-r-8" :style="{
              '--el-card-padding': '6px 16px',
              'min-height': answer ? 'auto' : '120px'
            }">
              <div>
                {{ answer }}
              </div>
            </el-card>
            <div>
              <el-button type="primary" @click="() => emit('replace', answer)">
                替换
              </el-button>
              <el-button type="primary" @click="reAnswerClick" :disabled="!answer || loading" :loading="loading">
                重新生成
              </el-button>
            </div>
          </div>

          <!-- 文本输入框 -->
          <div>
            <div class="ai-chat__operate p-16">
              <div class="text-center mb-8" v-if="loading">
                <el-button class="border-primary video-stop-button" @click="stopChat">
                  <app-icon iconName="app-video-stop" class="mr-8"></app-icon>
                  停止对话
                </el-button>
              </div>

              <div class="operate-textarea">
                <el-input ref="quickInputRef" v-model="inputValue" :autosize="{ minRows: 1, maxRows: 10 }"
                  type="textarea" placeholder="请输入您的问题..." :maxlength="100000"
                  class="chat-operate-textarea" />

                <div class="operate flex-between">
                  <div>
                    <slot name="userInput" />
                  </div>
                  <div class="flex align-center">
                    <el-button text class="sent-button" :disabled="!inputValue.trim() || loading" @click="handleSubmit">
                      <img v-show="!inputValue.trim() || loading" src="@/assets/icon_send.svg" alt="" />
                      <SendIcon v-show="inputValue.trim() && !loading" />
                    </el-button>
                  </div>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>



  </el-dialog>
</template>

<script setup lang="ts">
import { computed, reactive, ref } from 'vue'
import generatePromptAPI from '@/api/application/application'
import useStore from '@/stores';
const emit = defineEmits(['replace'])
const { user } = useStore()

const chatMessages = ref<Array<any>>([])

// 原始输入
const originalUserInput = ref<string>('')
const modelID = ref('')
const dialogVisible = ref(false)
const inputValue = ref<string>('')
const loading = ref<boolean>(false)

const promptTemplates = {
  INIT_TEMPLATE: `
请根据用户描述生成一个完整的AI角色人设模板:

用户需求：{userInput}

请按以下格式生成：

# 角色: 角色名称
角色概述和主要职责的一句话描述

## 目标：
角色的工作目标,如果有多目标可以分点列出,但建议更聚焦1-2个目标

## 技能：
1. 为了实现目标,角色需要具备的技能1
2. 为了实现目标,角色需要具备的技能2
3. 为了实现目标,角色需要具备的技能3

## 工作流：
1. 描述角色工作流程的第一步
2. 描述角色工作流程的第二步
3. 描述角色工作流程的第三步

## 输出格式：
如果对角色的输出格式有特定要求，可以在这里强调并举例说明想要的输出格式


## 限制：
描述角色在互动过程中需要遵循的限制条件1
描述角色在互动过程中需要遵循的限制条件2
描述角色在互动过程中需要遵循的限制条件3
  `
}



/**
 * 获取一个递归函数,处理流式数据
 * @param chat    每一条对话记录
 * @param reader  流数据
 * @param stream  是否是流式数据
 */
const getWrite = (reader: any) => {
  let tempResult = ''
  const answer = reactive({ content: '', 'role': 'ai' })
  chatMessages.value.push(answer)
  /**
   *
   * @param done  是否结束
   * @param value 值
   */
  const write_stream = ({ done, value }: { done: boolean; value: any }) => {
    try {
      if (done) {
        loading.value = false
        // console.log('结束')
        return
      }
      const decoder = new TextDecoder('utf-8')
      let str = decoder.decode(value, { stream: true })
      // 这里解释一下 start 因为数据流返回流并不是按照后端chunk返回 我们希望得到的chunk是data:{xxx}\n\n 但是它获取到的可能是 data:{ -> xxx}\n\n 总而言之就是 fetch不能保证每个chunk都说以data:开始 \n\n结束
      tempResult += str
      const split = tempResult.match(/data:.*}\n\n/g)
      if (split) {
        str = split.join('')
        tempResult = tempResult.replace(str, '')
      } else {
        return reader.read().then(write_stream)
      }
      // 这里解释一下 end
      if (str && str.startsWith('data:')) {
        if (split) {
          for (const index in split) {
            const chunk = JSON?.parse(split[index].replace('data:', ''))
            if (!chunk.is_end) {
              answer.content += chunk.content
            }
            if (chunk.is_end) {
              // 流处理成功 返回成功回调
              loading.value = false
              return Promise.resolve()
            }
          }
        }
      }
    } catch (e) {
      loading.value = false
      return Promise.reject(e)
    }
    return reader.read().then(write_stream)
  }

  return write_stream
}

const answer = computed(() => {
  const result = chatMessages.value[chatMessages.value.length - 1]

  if (result && result.role == 'ai') {
    return result.content
  }
  return ''
})


function generatePrompt(inputValue: any) {
  loading.value = true
  const workspaceId = user.getWorkspaceId() || 'default'
  chatMessages.value.push({ content: inputValue, role: 'user' })
  const requestData = {
    messages: chatMessages.value,
    prompt: promptTemplates.INIT_TEMPLATE
  }
  generatePromptAPI.generate_prompt(
    workspaceId, modelID.value, requestData).then(response => {
      const reader = response.body.getReader()
      reader.read().then(getWrite(reader))
    })

}

// 重新生成点击
const reAnswerClick = () => {
  if (originalUserInput.value) {
    generatePrompt('结果不满意,请按照格式,重新生成')
  }

}

const handleSubmit = () => {
  if (!originalUserInput.value) {
    originalUserInput.value = inputValue.value
  }
  generatePrompt(inputValue.value)
  inputValue.value = ''
}



const stopChat = () => {
  loading.value = false
  chatMessages.value = []
}


const open = (modelId: string) => {
  modelID.value = modelId
  dialogVisible.value = true
  originalUserInput.value = ''
  chatMessages.value = []
}

defineExpose({
  open
})

</script>

<style lang="scss" scoped>
.dialog-bg {
  border-radius: 8px;
  background: var(--dialog-bg-gradient-color);
  overflow: hidden;
  box-sizing: border-box;
}

.ai-chat {
  &__operate {
    position: relative;
    width: 100%;
    box-sizing: border-box;
    z-index: 10;

    :deep(.operate-textarea) {
      box-shadow: 0px 6px 24px 0px rgba(31, 35, 41, 0.08);
      background-color: #ffffff;
      border-radius: 8px;
      border: 1px solid #ffffff;
      box-sizing: border-box;

      &:has(.el-textarea__inner:focus) {
        border: 1px solid var(--el-color-primary);
      }

      .el-textarea__inner {
        border-radius: 8px !important;
        box-shadow: none;
        resize: none;
        padding: 13px 16px;
        box-sizing: border-box;
        min-height: 47px !important;
        height: 0;
      }

      .operate {
        padding: 6px 10px;

        .el-icon {
          font-size: 20px;
        }

        .sent-button {
          max-height: none;

          .el-icon {
            font-size: 24px;
          }
        }

        .el-loading-spinner {
          margin-top: -15px;

          .circular {
            width: 31px;
            height: 31px;
          }
        }
      }
    }

    .file-image {
      position: relative;
      overflow: inherit;

      .delete-icon {
        position: absolute;
        right: -5px;
        top: -5px;
        z-index: 1;
      }
    }

    .upload-tooltip-width {
      width: 300px;
    }
  }
}

@media only screen and (max-width: 768px) {
  .ai-chat {
    &__operate {
      position: fixed;
      bottom: 0;
      font-size: 1rem;

      .el-icon {
        font-size: 1.4rem !important;
      }
    }
  }
}

.popperUserInput {
  position: absolute;
  z-index: 999;
  left: 0;
  bottom: 50px;
  width: calc(100% - 50px);
  max-width: 400px;
}
</style>