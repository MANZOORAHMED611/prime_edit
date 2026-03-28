#ifndef SCHEMAVALIDATOR_H
#define SCHEMAVALIDATOR_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QJsonObject>

struct SchemaViolation {
    int line;           // 1-based
    int column;         // 0-based
    int endLine;
    int endColumn;
    QString rule;       // rule name that was violated
    QString message;    // human-readable message
    enum Severity { Error, Warning, Info } severity = Error;
};

struct SchemaRule {
    QString name;
    QString pattern;        // regex pattern to search for
    bool mustExist = true;  // true = pattern must be found, false = must NOT be found
    QString message;        // violation message
    SchemaViolation::Severity severity = SchemaViolation::Error;
    QString scope;          // "document", "block", "line"
};

class SchemaValidator : public QObject
{
    Q_OBJECT
public:
    explicit SchemaValidator(QObject *parent = nullptr);

    bool loadSchema(const QString &schemaPath);
    bool loadSchemaFromJson(const QJsonObject &schema);
    bool hasSchema() const { return !m_rules.isEmpty(); }
    QString schemaType() const { return m_type; }

    QVector<SchemaViolation> validate(const QString &text) const;

    // Built-in schemas
    static QJsonObject hadithSchema();

signals:
    void validationComplete(const QVector<SchemaViolation> &violations);

private:
    QString m_type;
    QVector<SchemaRule> m_rules;
};

#endif // SCHEMAVALIDATOR_H
