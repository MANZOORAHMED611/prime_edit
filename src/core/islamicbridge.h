#ifndef ISLAMICBRIDGE_H
#define ISLAMICBRIDGE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

struct HadithValidation {
    QString hadithText;
    QString grading;          // sahih, hasan, da'if
    QString source;           // e.g. "Bukhari, Book 1, Hadith 1"
    QString gradingAuthority; // e.g. "Al-Albani"
    bool isValid = false;
    QString error;
};

struct QuranValidation {
    QString surah;
    int ayah = 0;
    QString arabicText;
    QString translation;
    bool isValid = false;
    QString error;
};

class IslamicBridge : public QObject
{
    Q_OBJECT
public:
    explicit IslamicBridge(QObject *parent = nullptr);

    // Configuration
    void setEndpoint(const QString &url);
    QString endpoint() const { return m_endpoint; }
    bool isConnected() const { return m_connected; }

    // Hadith validation
    void validateHadith(const QString &text);

    // Quranic validation
    void validateQuranCitation(const QString &surah, int ayah);

    // Scholar lookup
    void lookupScholar(const QString &name);

    // Connectivity test
    void testConnection();

signals:
    void hadithValidated(const HadithValidation &result);
    void quranValidated(const QuranValidation &result);
    void scholarResult(const QString &name, const QString &info);
    void connectionTested(bool success, const QString &message);
    void errorOccurred(const QString &error);

private slots:
    void onHadithReply();
    void onQuranReply();
    void onScholarReply();
    void onConnectionReply();

private:
    QNetworkRequest makeRequest(const QString &path) const;

    QNetworkAccessManager *m_networkManager;
    QString m_endpoint = "http://localhost:8000";
    bool m_connected = false;
};

#endif // ISLAMICBRIDGE_H
