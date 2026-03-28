#include "schemavalidator.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QRegularExpression>

SchemaValidator::SchemaValidator(QObject *parent)
    : QObject(parent)
{
}

bool SchemaValidator::loadSchema(const QString &schemaPath)
{
    QFile file(schemaPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return false;
    }

    return loadSchemaFromJson(doc.object());
}

bool SchemaValidator::loadSchemaFromJson(const QJsonObject &schema)
{
    m_rules.clear();
    m_type = schema["type"].toString();

    QJsonArray rulesArray = schema["rules"].toArray();
    for (const QJsonValue &v : rulesArray) {
        if (v.isString()) {
            QString ruleName = v.toString();
            SchemaRule rule;
            rule.name = ruleName;

            if (ruleName == "must_include_grading") {
                rule.pattern =
                    "\\b(sahih|hasan|da'if|daif|weak|authentic"
                    "|sound|good|fabricated|mawdu)\\b";
                rule.mustExist = true;
                rule.message =
                    "Hadith citation missing grading authority "
                    "(sahih/hasan/da'if)";
                rule.severity = SchemaViolation::Error;
                rule.scope = "block";
            } else if (ruleName == "must_include_source") {
                rule.pattern =
                    "\\b(Bukhari|Muslim|Tirmidhi|Abu Dawud"
                    "|Nasa'i|Ibn Majah|Ahmad|Malik|Darimi"
                    "|Bayhaqi|Hakim|Tabarani)\\b";
                rule.mustExist = true;
                rule.message = "Citation missing source collection name";
                rule.severity = SchemaViolation::Warning;
                rule.scope = "block";
            } else if (ruleName == "no_weak_hadith") {
                rule.pattern =
                    "\\b(da'if|daif|weak|fabricated|mawdu)\\b";
                rule.mustExist = false;
                rule.message =
                    "Document contains weak/fabricated hadith reference";
                rule.severity = SchemaViolation::Warning;
                rule.scope = "line";
            } else if (ruleName == "must_include_arabic") {
                rule.pattern = "[\\x{0600}-\\x{06FF}]";
                rule.mustExist = true;
                rule.message = "Block missing Arabic text";
                rule.severity = SchemaViolation::Info;
                rule.scope = "block";
            } else if (ruleName == "quran_citation_format") {
                rule.pattern =
                    "\\b(Surah|surah|Quran)\\s+[\\w]+\\s*[:\\.]\\s*\\d+";
                rule.mustExist = true;
                rule.message =
                    "Quranic citation should follow format: "
                    "Surah Name:Ayah";
                rule.severity = SchemaViolation::Warning;
                rule.scope = "document";
            } else {
                continue;
            }

            m_rules.append(rule);
        } else if (v.isObject()) {
            QJsonObject ruleObj = v.toObject();
            SchemaRule rule;
            rule.name = ruleObj["name"].toString();
            rule.pattern = ruleObj["pattern"].toString();
            rule.mustExist = ruleObj.value("mustExist").toBool(true);
            rule.message = ruleObj["message"].toString();

            QString sev = ruleObj.value("severity").toString("error");
            if (sev == "warning") {
                rule.severity = SchemaViolation::Warning;
            } else if (sev == "info") {
                rule.severity = SchemaViolation::Info;
            } else {
                rule.severity = SchemaViolation::Error;
            }

            rule.scope = ruleObj.value("scope").toString("document");
            m_rules.append(rule);
        }
    }

    return !m_rules.isEmpty();
}

QVector<SchemaViolation> SchemaValidator::validate(
    const QString &text) const
{
    QVector<SchemaViolation> violations;

    for (const SchemaRule &rule : m_rules) {
        QRegularExpression regex(
            rule.pattern,
            QRegularExpression::CaseInsensitiveOption);
        if (!regex.isValid()) {
            continue;
        }

        if (rule.scope == "document") {
            bool found = regex.match(text).hasMatch();
            if (rule.mustExist && !found) {
                SchemaViolation v;
                v.line = 1;
                v.column = 0;
                v.endLine = 1;
                v.endColumn = 0;
                v.rule = rule.name;
                v.message = rule.message;
                v.severity = rule.severity;
                violations.append(v);
            } else if (!rule.mustExist && found) {
                QRegularExpressionMatchIterator it =
                    regex.globalMatch(text);
                while (it.hasNext()) {
                    QRegularExpressionMatch match = it.next();
                    int pos = match.capturedStart();
                    int lineNum = text.left(pos).count('\n') + 1;
                    int lineStart = text.lastIndexOf('\n', pos) + 1;

                    SchemaViolation v;
                    v.line = lineNum;
                    v.column = pos - lineStart;
                    v.endLine = lineNum;
                    v.endColumn = v.column + match.capturedLength();
                    v.rule = rule.name;
                    v.message = rule.message;
                    v.severity = rule.severity;
                    violations.append(v);
                }
            }
        } else if (rule.scope == "line") {
            QStringList lines = text.split('\n');
            for (int i = 0; i < lines.size(); ++i) {
                bool found = regex.match(lines[i]).hasMatch();
                if (!rule.mustExist && found) {
                    QRegularExpressionMatch match =
                        regex.match(lines[i]);
                    SchemaViolation v;
                    v.line = i + 1;
                    v.column = match.capturedStart();
                    v.endLine = i + 1;
                    v.endColumn =
                        v.column + match.capturedLength();
                    v.rule = rule.name;
                    v.message = rule.message;
                    v.severity = rule.severity;
                    violations.append(v);
                }
            }
        } else if (rule.scope == "block") {
            QStringList blocks =
                text.split(QRegularExpression("\n\\s*\n"));
            int lineOffset = 1;
            for (const QString &block : blocks) {
                bool found = regex.match(block).hasMatch();
                if (rule.mustExist && !found
                    && !block.trimmed().isEmpty()) {
                    SchemaViolation v;
                    v.line = lineOffset;
                    v.column = 0;
                    v.endLine = lineOffset;
                    int nlPos = block.indexOf('\n');
                    v.endColumn =
                        (nlPos > 0) ? nlPos : block.length();
                    v.rule = rule.name;
                    v.message = rule.message;
                    v.severity = rule.severity;
                    violations.append(v);
                }
                lineOffset += block.count('\n') + 2;
            }
        }
    }

    return violations;
}

QJsonObject SchemaValidator::hadithSchema()
{
    QJsonObject schema;
    schema["type"] = "hadith";
    QJsonArray rules;
    rules.append(QString("must_include_grading"));
    rules.append(QString("must_include_source"));
    rules.append(QString("no_weak_hadith"));
    schema["rules"] = rules;
    return schema;
}
