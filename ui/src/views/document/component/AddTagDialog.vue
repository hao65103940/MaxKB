<template>
  <el-dialog
    v-model="dialogVisible"
    :title="$t('views.document.tag.add')"
    :before-close="close"
  >
    <el-form
      ref="FormRef"
      :model="{ tags }"
      label-position="top"
      require-asterisk-position="right"
      @submit.prevent
    >
      <div class="flex-between flex" v-for="(tag, index) in tags" :key="tag">
        <el-form-item :label="index === 0? $t('views.document.tag.key') : ''"
                      :prop="`tags.${index}.key`"
                      style="width: 50%"
                      :rules="{ required: true, message: $t('views.document.tag.requiredMessage1'), trigger: 'blur' }">
          <el-select v-model="tag.key" @change="tagKeyChange(tag)" filterable>
            <el-option v-for="op in keyOptions" :key="op" :value="op.key"
                       :label="op.key"></el-option>
          </el-select>
        </el-form-item>
        <el-form-item :label="index === 0? $t('views.document.tag.value') : ''"
                      :prop="`tags.${index}.value`"
                      style="width: 50%"
                      :rules="{ required: true, message: $t('views.document.tag.requiredMessage2'), trigger: 'blur' }">
          <el-select v-model="tag.value" filterable>
            <el-option v-for="op in tag.valueOptions" :key="op" :value="op.id"
                       :label="op.value"></el-option>
          </el-select>
        </el-form-item>
      </div>
    </el-form>


    <div class="mt-20">
      <el-button link type="primary" @click="add">
        <AppIcon iconName="app-add-outlined" class="mr-4"/>
        {{ $t('common.add') }}
      </el-button>
    </div>
    <template #footer>
      <div class="dialog-footer">
        <el-button @click="close">{{ $t('common.cancel') }}</el-button>
        <el-button type="primary" @click="submit">{{ $t('common.confirm') }}</el-button>
      </div>
    </template>
  </el-dialog>
</template>
<script setup lang="ts">
import { ref } from 'vue'
import AppIcon from "@/components/app-icon/AppIcon.vue";

const emit = defineEmits(['addTags'])
const props = defineProps({
  knowledgeTags: {
    type: Array,
    default: () => []
  }
})

const FormRef = ref()
const dialogVisible = ref<boolean>(false)
const tags = ref<Array<any>>([])
const keyOptions = ref()

const add = () => {
  tags.value.push({})
}

function tagKeyChange(tag: any) {
  const currentKeyOption = keyOptions.value.find((op: any) => op.key === tag.key)
  tag.valueOptions = currentKeyOption ? currentKeyOption.values : []
  tag.value = null
}

const submit = () => {
  FormRef.value.validate((valid: boolean) => {
    if (!valid) return
    emit('addTags', tags.value.map(tag => tag.value))
  })
}

const open = () => {
  dialogVisible.value = true
  tags.value = [{}]
  keyOptions.value = props.knowledgeTags
}

const close = () => {
  dialogVisible.value = false
}

defineExpose({open, close})
</script>
<style lang="scss" scoped></style>
