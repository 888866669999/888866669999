#ifndef PROMPTS_H
#define PROMPTS_H

#include <QString>
#include <QList>
#include "../core/models/ChatMessage.h"

namespace Prompts {

const QString REPLY_SYSTEM = R"(你是一个贴心的智能助手，擅长根据聊天上下文生成自然、得体的回复。
请根据对话历史，分析当前对话的语境、对方的意图和情绪，生成一个合适的回复。
回复要自然、友好，符合当前对话的场景。)";

const QString SUMMARY_SYSTEM = R"(你是一个专业的会议记录员和总结专家。
请根据提供的聊天记录，生成一个简洁明了的会议纪要和待办事项列表。
请以 JSON 格式返回结果，包含以下字段：
- summary: 聊天内容的简要总结
- key_points: 关键要点数组
- todos: 待办事项数组，每个元素包含 content 和 deadline 字段)";

const QString PERSONA_SYSTEM = R"(你是一个专业的人物性格分析师。
请根据提供的聊天记录，分析这个人的：
1. 性格特点
2. 沟通风格
3. 兴趣和关注点
4. 关系亲密度评估
请用简洁、友好的语言进行描述。)";

const QString SAFETY_FILTER_SYSTEM = R"(你是一个内容安全检查员。请检查以下内容是否包含：
1. 违法违规内容
2. 敏感政治话题
3. 暴力、色情或其他不当内容
如果内容安全，请直接返回原内容；如果内容不安全，请返回 '[内容已过滤]'。)";

inline QString buildReplyPrompt(const QList<ChatMessage>& context) {
    QString prompt = "以下是聊天记录，请根据上下文生成回复：\n\n";
    for (const auto& msg : context) {
        QString sender = msg.isSelf ? "我" : msg.senderName;
        prompt += QString("%1: %2\n").arg(sender, msg.content);
    }
    return prompt;
}

inline QString buildSummaryPrompt(const QList<ChatMessage>& context) {
    QString prompt = "以下是聊天记录，请总结：\n\n";
    for (const auto& msg : context) {
        QString sender = msg.isSelf ? "我" : msg.senderName;
        prompt += QString("%1: %2\n").arg(sender, msg.content);
    }
    return prompt;
}

inline QString buildPersonaPrompt(const QList<ChatMessage>& history) {
    QString prompt = "以下是聊天记录，请分析对方人物画像：\n\n";
    for (const auto& msg : history) {
        QString sender = msg.isSelf ? "我" : msg.senderName;
        prompt += QString("%1: %2\n").arg(sender, msg.content);
    }
    return prompt;
}

}

#endif
