<template>
  <el-dialog
    v-model="dialogVisible"
    :title="$t('views.document.tag.create')"
    :before-close="close"
  >
    <el-form
      ref="FormRef"
      :model="{ tags }"
      label-position="top"
      require-asterisk-position="right"
      @submit.prevent
    >
      <div class="flex-between" v-for="(tag, index) in tags" :key="tag">
        <el-form-item :label="index === 0? $t('views.document.tag.key') : ''"
                      :prop="`tags.${index}.key`"
                      style="width: 50%"
                      :rules="{ required: true, message: $t('views.document.tag.requiredMessage1'), trigger: 'blur' }">
          <el-input v-model="tag.key" :disabled="currentTagKey"></el-input>
        </el-form-item>
        <el-form-item :label="index === 0? $t('views.document.tag.value') : ''"
                      :prop="`tags.${index}.value`"
                      style="width: 50%"
                      :rules="{ required: true, message: $t('views.document.tag.requiredMessage2'), trigger: 'blur' }">
          <el-input v-model="tag.value"></el-input>
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
import { computed, ref } from 'vue'
import AppIcon from "@/components/app-icon/AppIcon.vue";
import { useRoute } from "vue-router";
import { loadSharedApi } from "@/utils/dynamics-api/shared-api.ts";
import { cloneDeep } from "lodash";

const route = useRoute()
const {
  params: {id}, // id为knowledgeID
} = route as any
const emit = defineEmits(['refresh'])

const apiType = computed(() => {
  if (route.path.includes('shared')) {
    return 'systemShare'
  } else if (route.path.includes('resource-management')) {
    return 'systemManage'
  } else {
    return 'workspace'
  }
})

const FormRef = ref()
const loading = ref(false)
const dialogVisible = ref<boolean>(false)
const currentTagKey = ref(null)
const tags = ref<Array<any>>([])

const add = () => {
  if (currentTagKey.value) {
    tags.value.push({key: currentTagKey.value})
  } else {
    tags.value.push({})
  }
}

const submit = () => {
  FormRef.value.validate((valid: boolean) => {
    if (!valid) return
    loadSharedApi({type: 'knowledge', systemType: apiType.value})
      .postTags(id, tags.value, loading)
      .then((res: any) => {
        close()
        emit('refresh')
      })
  })
}

const open = (row?: any) => {
  const currentRow = cloneDeep(row)
  dialogVisible.value = true
  currentTagKey.value = currentRow ? currentRow.key : null
  tags.value = currentRow ? [{...{key: currentRow.key}}] : [{}]
}

const close = () => {
  dialogVisible.value = false
}

defineExpose({open, close})
</script>
<style lang="scss" scoped></style>
