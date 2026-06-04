## QQ Chat Exporter

将 QQ 聊天记录导出为 HTML、JSON、TXT 格式。支持定时备份、批量导出、表情包导出。

[![hero](https://github.com/shuakami/qq-chat-exporter/raw/9959f84b/image.png?raw=true)](https://github.com/shuakami/qq-chat-exporter/blob/9959f84b/image.png?raw=true)

## 文档

访问 [https://shuakami.github.io/qq-chat-exporter/](https://shuakami.github.io/qq-chat-exporter/) 查看使用文档。

## 快速开始

1. 从 [Releases](https://github.com/shuakami/qq-chat-exporter/releases) 下载
2. 运行 `launcher-user.bat` (Windows) 或 `./launcher-user.sh` (Linux)
3. 用 QQ 扫码登录
4. 复制控制台的 Token
5. 打开 `http://localhost:40653/qce-v4-tool`

### Docker NapCat 部署

如果已有 Docker 部署的 NapCat（Shell 模式），可以作为插件直接挂载，无需桌面 QQNT 环境。

详见 [Docker NapCat 部署指南](https://github.com/shuakami/qq-chat-exporter/blob/master/docs/docker-napcat-deployment.md) 。

## 相关项目

如果导出聊天记录后，想深入分析聊天内容可以试试 [ChatLab](https://chatlab.fun/cn)

也可以试试 [QQChatAnalyzer](https://github.com/CutrelyAlex/QQChatAnalyzer) - 支持个人分析、群聊分析、社交网络可视化和 AI 摘要

还可以试试 [QQ-Chat-AI-Analyzer](https://github.com/JUSTMONIKA2022/QQ-Chat-AI-Analyzer) - 基于 AI 的群聊消息总结分析工具，可生成年度报告

如果需要 Python API 封装，可以使用 [napcat-qce-python](https://github.com/streetartist/napcat-qce-python)

## 致谢

感谢 [NapCatQQ](https://github.com/NapNeko/NapCatQQ) 团队提供的框架支持。

[

![](https://camo.githubusercontent.com/90cc76b026157f6e85a87dd96d9d5353422af601b7df6105ec4f713573f5ea1c/68747470733a2f2f636f6e747269622e726f636b732f696d6167653f7265706f3d736875616b616d692f71712d636861742d6578706f72746572266d61783d313226636f6c756d6e733d3132)

](https://github.com/shuakami/qq-chat-exporter/graphs/contributors)

## 许可证

[GPL-3.0](https://github.com/shuakami/qq-chat-exporter/blob/main/LICENSE)