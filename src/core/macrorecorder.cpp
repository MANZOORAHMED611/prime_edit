#include "macrorecorder.h"
#include "utils/settings.h"
#include <QApplication>
#include <QKeyEvent>
#include <QTimer>
#include <QWidget>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QFile>

MacroRecorder::MacroRecorder()
    : m_recording(false)
    , m_playingBack(false)
{
    loadMacrosFromSettings();
}

MacroRecorder::~MacroRecorder()
{
    saveMacrosToSettings();
}

MacroRecorder& MacroRecorder::instance()
{
    static MacroRecorder instance;
    return instance;
}

void MacroRecorder::startRecording()
{
    if (m_recording) return;

    m_currentMacro.clear();
    m_recording = true;
    emit recordingStarted();
}

void MacroRecorder::stopRecording()
{
    if (!m_recording) return;

    m_recording = false;
    emit recordingStopped();
}

void MacroRecorder::clear()
{
    m_currentMacro.clear();
}

void MacroRecorder::recordKeyEvent(QKeyEvent *event)
{
    if (!m_recording || m_playingBack) return;

    // Don't record macro control keys
    if (event->key() == Qt::Key_F9 || event->key() == Qt::Key_F10) {
        return;
    }

    MacroEvent macroEvent;
    macroEvent.type = (event->type() == QEvent::KeyPress) ? MacroEvent::KeyPress : MacroEvent::KeyRelease;
    macroEvent.key = event->key();
    macroEvent.modifiers = event->modifiers();
    macroEvent.text = event->text();

    m_currentMacro.append(macroEvent);
}

void MacroRecorder::recordText(const QString &text)
{
    if (!m_recording || m_playingBack || text.isEmpty()) return;

    MacroEvent macroEvent;
    macroEvent.type = MacroEvent::TextInput;
    macroEvent.text = text;

    m_currentMacro.append(macroEvent);
}

void MacroRecorder::playback(QWidget *target)
{
    if (m_playingBack || m_recording || !target) return;
    if (m_currentMacro.isEmpty()) return;

    m_playingBack = true;
    emit playbackStarted();

    // Play back events synchronously for simplicity
    // For better user experience, could use QTimer for async playback
    for (const MacroEvent &event : m_currentMacro) {
        QKeyEvent *keyEvent = nullptr;

        if (event.type == MacroEvent::KeyPress) {
            keyEvent = new QKeyEvent(QEvent::KeyPress, event.key, event.modifiers, event.text);
            QApplication::sendEvent(target, keyEvent);
            delete keyEvent;
        } else if (event.type == MacroEvent::KeyRelease) {
            keyEvent = new QKeyEvent(QEvent::KeyRelease, event.key, event.modifiers, event.text);
            QApplication::sendEvent(target, keyEvent);
            delete keyEvent;
        } else if (event.type == MacroEvent::TextInput) {
            // Send text as a series of key presses
            for (const QChar &ch : event.text) {
                keyEvent = new QKeyEvent(QEvent::KeyPress, 0, Qt::NoModifier, QString(ch));
                QApplication::sendEvent(target, keyEvent);
                delete keyEvent;
            }
        }

        // Small delay for visual feedback
        QApplication::processEvents();
    }

    m_playingBack = false;
    emit playbackStopped();
}

void MacroRecorder::stopPlayback()
{
    m_playingBack = false;
    emit playbackStopped();
}

void MacroRecorder::saveMacro(const QString &name)
{
    if (name.isEmpty() || m_currentMacro.isEmpty()) return;

    m_savedMacros[name] = m_currentMacro;
    saveMacrosToSettings();
    emit macroSaved(name);
}

void MacroRecorder::loadMacro(const QString &name)
{
    if (!m_savedMacros.contains(name)) return;

    m_currentMacro = m_savedMacros[name];
    emit macroLoaded(name);
}

QStringList MacroRecorder::savedMacroNames() const
{
    return m_savedMacros.keys();
}

void MacroRecorder::deleteMacro(const QString &name)
{
    m_savedMacros.remove(name);
    saveMacrosToSettings();
}

QString MacroRecorder::currentMacroDescription() const
{
    if (m_currentMacro.isEmpty()) {
        return tr("No macro recorded");
    }

    QString desc;
    int count = qMin(5, m_currentMacro.size());
    for (int i = 0; i < count; ++i) {
        if (i > 0) desc += ", ";
        desc += eventToString(m_currentMacro[i]);
    }

    if (m_currentMacro.size() > 5) {
        desc += QString("... (%1 events total)").arg(m_currentMacro.size());
    }

    return desc;
}

void MacroRecorder::loadMacrosFromSettings()
{
    m_savedMacros.clear();

    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString filePath = configPath + "/macros.json";

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) return;

    QJsonObject root = doc.object();
    for (auto it = root.begin(); it != root.end(); ++it) {
        QVector<MacroEvent> events;
        QJsonArray arr = it.value().toArray();
        for (const QJsonValue &val : arr) {
            QJsonObject obj = val.toObject();
            MacroEvent event;
            event.type = static_cast<MacroEvent::Type>(obj["type"].toInt());
            event.key = obj["key"].toInt();
            event.modifiers = static_cast<Qt::KeyboardModifiers>(obj["modifiers"].toInt());
            event.text = obj["text"].toString();
            events.append(event);
        }
        m_savedMacros[it.key()] = events;
    }
}

void MacroRecorder::saveMacrosToSettings()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configPath);
    QString filePath = configPath + "/macros.json";

    QJsonObject root;
    for (auto it = m_savedMacros.constBegin(); it != m_savedMacros.constEnd(); ++it) {
        QJsonArray arr;
        for (const MacroEvent &event : it.value()) {
            QJsonObject obj;
            obj["type"] = static_cast<int>(event.type);
            obj["key"] = event.key;
            obj["modifiers"] = static_cast<int>(event.modifiers);
            obj["text"] = event.text;
            arr.append(obj);
        }
        root[it.key()] = arr;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) return;

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Compact));
}

QString MacroRecorder::eventToString(const MacroEvent &event) const
{
    QString result;

    if (event.type == MacroEvent::TextInput) {
        return "Text: \"" + event.text + "\"";
    }

    // Key modifiers
    if (event.modifiers & Qt::ControlModifier) result += "Ctrl+";
    if (event.modifiers & Qt::ShiftModifier) result += "Shift+";
    if (event.modifiers & Qt::AltModifier) result += "Alt+";
    if (event.modifiers & Qt::MetaModifier) result += "Meta+";

    // Key name
    QString keyName = QKeySequence(event.key).toString();
    if (keyName.isEmpty()) {
        keyName = event.text.isEmpty() ? QString("Key_%1").arg(event.key) : event.text;
    }

    result += keyName;

    if (event.type == MacroEvent::KeyPress) {
        result += " ↓";
    } else {
        result += " ↑";
    }

    return result;
}
