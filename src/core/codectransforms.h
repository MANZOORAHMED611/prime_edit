#ifndef CODECTRANSFORMS_H
#define CODECTRANSFORMS_H

#include <QString>

#include <optional>

// Pure encoders, decoders and hashes behind the Notepad++-parity Tools menu
// (MD5/SHA, MIME Tools, Converter). Text is taken as UTF-8 bytes. A decoder
// returns nullopt for malformed input, or for bytes that are not valid UTF-8,
// so the caller can refuse instead of replacing the text with garbage.
namespace CodecTransforms {

enum class HashAlgorithm { Md5, Sha1, Sha256, Sha512 };

// Lowercase hex digest of the text's UTF-8 bytes.
QString hashText(const QString &text, HashAlgorithm algorithm);
// Lowercase hex digest of the file's bytes; opens it read-only. nullopt when
// it cannot be read.
std::optional<QString> hashFile(const QString &path, HashAlgorithm algorithm);

QString base64Encode(const QString &text);
// Standard alphabet; whitespace (line wrapping) is ignored.
std::optional<QString> base64Decode(const QString &text);

// Percent-encodes everything except A-Z a-z 0-9 - . _ ~ (RFC 3986 unreserved).
QString urlEncode(const QString &text);
// Every % must start a two-digit hex escape; + stays a plus sign.
std::optional<QString> urlDecode(const QString &text);

// Uppercase hex pairs with no separator, as the Notepad++ Converter writes.
QString asciiToHex(const QString &text);
// Whitespace between digits is ignored; the digit count must be even.
std::optional<QString> hexToAscii(const QString &text);

} // namespace CodecTransforms

#endif // CODECTRANSFORMS_H
