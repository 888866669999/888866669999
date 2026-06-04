# 微信平台技术说明

## 概述

本文档记录微信 4.x 平台的技术细节，供 WeChatAdapter 实现参考。

---

## 数据库信息

- **加密方式**: SQLCipher 4 (AES-256-CBC + HMAC-SHA512)
- **密钥来源**: 从 WeChatWin.dll 进程内存中扫描
- **密钥特征**: `x'<64hex_enc_key><32hex_salt>'`
- **默认数据目录**: `%USERPROFILE%\Documents\WeChat Files`

---

## 主要数据库文件

| 文件名 | 用途 |
|--------|------|
| MSG.db | 消息记录 |
| MicroMsg.db | 联系人信息 |
| MediaMSG.db | 媒体文件索引 |

---

## 密钥提取方法

参考 [wechat-decrypt.md](../../../wechat-decrypt.md) 文档。

---

## WAL 文件监控

微信使用 SQLite WAL 模式，需监控 `.db-wal` 文件变化。
