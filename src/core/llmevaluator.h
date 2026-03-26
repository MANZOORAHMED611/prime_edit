#ifndef LLMEVALUATOR_H
#define LLMEVALUATOR_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

struct EvalEndpoint {
    QString name = "Ollama";
    QString url = "http://localhost:11434/api/generate";
    QString model = "llama3";
    QString apiKey;
    QString systemPrompt = "You are a helpful editor. Improve the following text. Return only the improved text, no explanations.";
    enum Type { Ollama, OpenAICompatible } type = Ollama;
};

class LLMEvaluator : public QObject
{
    Q_OBJECT
public:
    explicit LLMEvaluator(QObject *parent = nullptr);

    void evaluate(const QString &text, const EvalEndpoint &endpoint);
    void cancel();

    static EvalEndpoint loadEndpoint();
    static void saveEndpoint(const EvalEndpoint &endpoint);

signals:
    void resultReady(const QString &result);
    void errorOccurred(const QString &error);

private slots:
    void onReplyFinished();
    void onReadyRead();

private:
    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_currentReply = nullptr;
    QString m_accumulatedResponse;
    EvalEndpoint m_currentEndpoint;
};

#endif // LLMEVALUATOR_H
