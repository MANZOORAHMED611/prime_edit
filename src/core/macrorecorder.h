#ifndef MACRORECORDER_H
#define MACRORECORDER_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QKeyEvent>
#include <QMap>

class MacroRecorder : public QObject
{
    Q_OBJECT

public:
    static MacroRecorder& instance();

    // Recording control
    void startRecording();
    void stopRecording();
    bool isRecording() const { return m_recording; }

    // Playback
    void playback(QWidget *target);
    void stopPlayback();
    bool isPlayingBack() const { return m_playingBack; }

    // Record an event
    void recordKeyEvent(QKeyEvent *event);
    void recordText(const QString &text);

    // Macro management
    void saveMacro(const QString &name);
    void loadMacro(const QString &name);
    QStringList savedMacroNames() const;
    void deleteMacro(const QString &name);

    // Clear current recording
    void clear();

    // Get current macro for display
    QString currentMacroDescription() const;

signals:
    void recordingStarted();
    void recordingStopped();
    void playbackStarted();
    void playbackStopped();
    void macroSaved(const QString &name);
    void macroLoaded(const QString &name);

private:
    MacroRecorder();
    ~MacroRecorder() override;
    MacroRecorder(const MacroRecorder&) = delete;
    MacroRecorder& operator=(const MacroRecorder&) = delete;

    struct MacroEvent {
        enum Type {
            KeyPress,
            KeyRelease,
            TextInput
        };

        Type type;
        int key;
        Qt::KeyboardModifiers modifiers;
        QString text;

        MacroEvent() : type(KeyPress), key(0), modifiers(Qt::NoModifier) {}
    };

    bool m_recording;
    bool m_playingBack;
    QVector<MacroEvent> m_currentMacro;
    QMap<QString, QVector<MacroEvent>> m_savedMacros;

    void loadMacrosFromSettings();
    void saveMacrosToSettings();
    QString eventToString(const MacroEvent &event) const;
};

#endif // MACRORECORDER_H
