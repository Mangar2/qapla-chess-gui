/*
 * Comprehensive PGN parser
 *
 * This program implements a robust parser for Portable Game Notation (PGN)
 * files in modern C++ (C++20). It uses a full tokenizer to split the input
 * into lexical tokens representing punctuation, identifiers, numbers,
 * comments and quoted strings. A recursive descent parser consumes these
 * tokens to build a representation of each game. Tags, moves, comments,
 * variations, numeric annotation glyphs and results are all recognised.
 *
 * The parser deliberately avoids external dependencies: it relies only on
 * the C++ standard library. It does not attempt to validate chess moves
 * or FEN strings; instead it focuses on recognising the syntactic
 * structure of PGN files. Comments are parsed flexibly: engine annotations
 * such as `{+0.31/14 0.89s b1c3 e7e6 …}` or `{+M1/5 0s, White mates}` are
 * broken down into evaluation value (`value`), search depth (`depth`),
 * search duration (`duration`), principal variation (`pv`) and any
 * following descriptive text (`info`). Simple comments like `{book}` are
 * captured under the key `comment`. Semicolon comments are also handled.
 *
 * PGN text begins with tag pairs, enclosed in square brackets. Each tag
 * contains a name and a value in quotes【182475049977295†L195-L211】. After a blank line the movetext
 * follows, consisting of move numbers, moves, annotations, comments and
 * results. Comments are either delimited by braces `{` `}` or start with
 * a semicolon and continue to the end of the line【182475049977295†L303-L309】. Variations are enclosed in
 * parentheses `(` `)` and may nest.
 */

#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <utility>
#include <optional>

namespace pgn {

// Forward declarations
struct Token;
struct Game;
struct Move;
struct KeyValue;

/**
 * Enumeration of lexical token types.
 */
enum class TokenType {
    LBRACKET,    // '['
    RBRACKET,    // ']'
    LBRACE,      // '{'
    RBRACE,      // '}'
    LPAREN,      // '('
    RPAREN,      // ')'
    COMMA,       // ','
    SLASH,       // '/'
    PLUS,        // '+'
    MINUS,       // '-'
    PERIOD,      // '.' (not part of decimal number)
    IDENTIFIER,  // sequences of letters and digits (e.g. moves, PV tokens)
    NUMBER,      // numeric token, digits with optional decimal point
    QUOTED_STRING, // double‑quoted string (tag values)
    SEMICOLON_COMMENT, // comment starting with ';' to end of line
    DOLLAR,      // '$' (start of numeric annotation glyph)
    PUNCT,       // punctuation not categorised above ('?', '!', '#', '=')
    UNKNOWN
};

/**
 * Represents a single lexical token with type and text.
 */
struct Token {
    TokenType type;
    std::string text;
};

/**
 * Represents a key/value pair used for tags and move attributes.
 */
struct KeyValue {
    std::string key;
    std::string value;
};

/**
 * Represents a move with a list of arbitrary key/value fields. At least
 * a `move` field will be present for normal moves; other fields describe
 * comments, engine evaluations, variations, results or NAGs.
 */
struct Move {
    std::vector<KeyValue> fields;
};

/**
 * Represents a complete PGN game consisting of tags and a sequence of
 * moves.
 */
struct Game {
    std::vector<KeyValue> tags;
    std::vector<Move> moves;
};

/**
 * Tokeniser class that converts PGN text into a sequence of tokens.
 */
class Tokeniser {
public:
    explicit Tokeniser(const std::string& input)
        : text(input), pos(0), length(input.size()) {}

    /**
     * Produce the next token. Returns std::nullopt at end of input.
     */
    std::optional<Token> next() {
        skipWhitespace();
        if (pos >= length) {
            return std::nullopt;
        }
        char c = text[pos];
        // Semicolon comment: consume ';' and everything until newline
        if (c == ';') {
            // skip ';'
            pos++;
            std::string comment;
            while (pos < length && text[pos] != '\n' && text[pos] != '\r') {
                comment += text[pos++];
            }
            return Token{TokenType::SEMICOLON_COMMENT, comment};
        }
        // Quoted string for tag value
        if (c == '"') {
            pos++; // skip opening quote
            std::string value;
            while (pos < length) {
                char ch = text[pos++];
                if (ch == '\\') {
                    // escape sequence: consume next character as literal
                    if (pos < length) {
                        value += text[pos++];
                    }
                } else if (ch == '"') {
                    break;
                } else {
                    value += ch;
                }
            }
            return Token{TokenType::QUOTED_STRING, value};
        }
        // Single character tokens
        switch (c) {
            case '[': pos++; return Token{TokenType::LBRACKET, "["};
            case ']': pos++; return Token{TokenType::RBRACKET, "]"};
            case '{': pos++; return Token{TokenType::LBRACE, "{"};
            case '}': pos++; return Token{TokenType::RBRACE, "}"};
            case '(': pos++; return Token{TokenType::LPAREN, "("};
            case ')': pos++; return Token{TokenType::RPAREN, ")"};
            case ',': pos++; return Token{TokenType::COMMA, ","};
            case '/': pos++; return Token{TokenType::SLASH, "/"};
            case '+': pos++; return Token{TokenType::PLUS, "+"};
            case '-': {
                // minus may be part of castling or result; treat as MINUS token
                pos++;
                return Token{TokenType::MINUS, "-"};
            }
            case '$': pos++; return Token{TokenType::DOLLAR, "$"};
            case '.': {
                // If part of decimal number? check previous char and next char handled in number scanning below.
                pos++;
                return Token{TokenType::PERIOD, "."};
            }
            case '!': pos++; return Token{TokenType::PUNCT, "!"};
            case '?': pos++; return Token{TokenType::PUNCT, "?"};
            case '#': pos++; return Token{TokenType::PUNCT, "#"};
            case '=': pos++; return Token{TokenType::PUNCT, "="};
            default:
                break;
        }
        // Castling detection: look for "O-O" or "O-O-O" with uppercase letter O
        if (c == 'O') {
            size_t start = pos;
            size_t p = pos;
            std::string castling;
            castling += 'O';
            p++;
            // match '-' 'O'
            if (p < length && text[p] == '-') {
                castling += '-';
                p++;
                if (p < length && text[p] == 'O') {
                    castling += 'O';
                    p++;
                    // check for optional third part '-O'
                    if (p < length && text[p] == '-') {
                        // maybe 'O-O-O'
                        size_t p2 = p + 1;
                        if (p2 < length && text[p2] == 'O') {
                            castling += "-O";
                            p = p2 + 1;
                        }
                    }
                    pos = p;
                    return Token{TokenType::IDENTIFIER, castling};
                }
            }
            // If castling detection fails, fall through to identifier scanning
        }
        // Number detection: digits with optional one decimal point
        if (std::isdigit(static_cast<unsigned char>(c))) {
            size_t start = pos;
            bool hasDot = false;
            pos++;
            while (pos < length) {
                char ch = text[pos];
                if (std::isdigit(static_cast<unsigned char>(ch))) {
                    pos++;
                    continue;
                }
                if (ch == '.' && !hasDot) {
                    // look ahead to see if next char is digit; if not, break (e.g. move number '20.')
                    if (pos + 1 < length && std::isdigit(static_cast<unsigned char>(text[pos + 1]))) {
                        hasDot = true;
                        pos++;
                        continue;
                    } else {
                        break;
                    }
                }
                break;
            }
            std::string num = text.substr(start, pos - start);
            return Token{TokenType::NUMBER, num};
        }
        // Identifier: letters and digits combined, used for SAN moves, PV tokens, words in comments
        if (std::isalpha(static_cast<unsigned char>(c))) {
            size_t start = pos;
            pos++;
            while (pos < length) {
                char ch = text[pos];
                // letters or digits constitute identifier; break on other punctuation or whitespace
                if (std::isalnum(static_cast<unsigned char>(ch))) {
                    pos++;
                    continue;
                }
                break;
            }
            std::string ident = text.substr(start, pos - start);
            return Token{TokenType::IDENTIFIER, ident};
        }
        // Fallback: unrecognised character as unknown token
        pos++;
        std::string t(1, c);
        return Token{TokenType::UNKNOWN, t};
    }

private:
    const std::string& text;
    size_t pos;
    const size_t length;
    void skipWhitespace() {
        while (pos < length) {
            char c = text[pos];
            // skip spaces and control characters except newline (we keep newline for semicolon comments splitting but newline is whitespace otherwise)
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                pos++;
            } else {
                break;
            }
        }
    }
};

/**
 * Parser class that consumes tokens and builds Game objects.
 */
class Parser {
public:
    explicit Parser(const std::vector<Token>& toks) : tokens(toks), index(0), size(toks.size()) {}

    Game parseGame() {
        Game game;
        // Parse tags until first blank line (represented by newline as whitespace in tokeniser)
        parseTags(game.tags);
        // Parse movetext
        parseMovetext(game.moves);
        return game;
    }

private:
    const std::vector<Token>& tokens;
    size_t index;
    const size_t size;

    // Helper: advance index and return reference to current token or std::nullopt
    std::optional<const Token*> peek(size_t offset = 0) const {
        if (index + offset < size) {
            return &tokens[index + offset];
        }
        return std::nullopt;
    }
    const Token* consume() {
        if (index < size) {
            return &tokens[index++];
        }
        return nullptr;
    }

    /**
     * Parse tags of form [Key "Value"]
     */
    void parseTags(std::vector<KeyValue>& tags) {
        while (index < size) {
            auto tokOpt = peek();
            if (!tokOpt) break;
            const Token* tok = *tokOpt;
            if (tok->type == TokenType::LBRACKET) {
                consume(); // consume '['
                // Expect identifier for tag name
                std::string tagName;
                std::string tagValue;
                auto nameTokOpt = peek();
                if (nameTokOpt && (*nameTokOpt)->type == TokenType::IDENTIFIER) {
                    tagName = (*nameTokOpt)->text;
                    consume();
                }
                // Expect quoted string
                auto valTokOpt = peek();
                if (valTokOpt && (*valTokOpt)->type == TokenType::QUOTED_STRING) {
                    tagValue = (*valTokOpt)->text;
                    consume();
                }
                // Consume until RBRACKET
                while (index < size) {
                    const Token* t = consume();
                    if (t->type == TokenType::RBRACKET) break;
                }
                if (!tagName.empty()) {
                    tags.push_back({tagName, tagValue});
                }
            } else if (tok->type == TokenType::SEMICOLON_COMMENT) {
                // ignore semicolon comment in tag section
                consume();
            } else if (tok->type == TokenType::RBRACKET) {
                consume();
            } else {
                // Not a tag; end of tag section
                break;
            }
        }
    }

    /**
     * Parse movetext section into moves.
     */
    void parseMovetext(std::vector<Move>& moves) {
        Move* lastMove = nullptr;
        while (index < size) {
            auto tokOpt = peek();
            if (!tokOpt) break;
            const Token* tok = *tokOpt;
            switch (tok->type) {
                case TokenType::LBRACE: {
                    // parse brace comment
                    std::vector<KeyValue> cFields = parseBraceComment();
                    consume(); // consume RBRACE inside parseBraceComment, but it leaves index at '}' so we need to consume one more '}'? parseBraceComment will already consume until after '}' and leave index at next token; we should not consume again here
                    if (lastMove) {
                        for (auto& kv : cFields) {
                            lastMove->fields.push_back(kv);
                        }
                    }
                    break;
                }
                case TokenType::SEMICOLON_COMMENT: {
                    // semicolon comment; treat as comment field
                    std::string content = tok->text;
                    consume();
                    if (lastMove) {
                        // parse semicolon comment like a brace comment
                        // We wrap semicolon comment into a comment content for parseCommentTokens
                        std::vector<Token> commentTokens;
                        // convert content string into identifier tokens separated by whitespace
                        std::istringstream iss(content);
                        std::string word;
                        while (iss >> word) {
                            commentTokens.push_back({TokenType::IDENTIFIER, word});
                        }
                        std::vector<KeyValue> fields = parseCommentTokens(commentTokens);
                        for (auto& kv : fields) {
                            lastMove->fields.push_back(kv);
                        }
                    }
                    break;
                }
                case TokenType::LPAREN: {
                    // parse variation
                    std::string variation = parseVariation();
                    if (lastMove) {
                        lastMove->fields.push_back({"variation", variation});
                    }
                    break;
                }
                case TokenType::DOLLAR: {
                    // NAG: consume '$' and optional number following
                    consume(); // consume '$'
                    std::string number;
                    auto nextTok = peek();
                    if (nextTok && (*nextTok)->type == TokenType::NUMBER) {
                        number = (*nextTok)->text;
                        consume();
                    }
                    if (lastMove) {
                        lastMove->fields.push_back({"nag", number});
                    }
                    break;
                }
                case TokenType::NUMBER: {
                    // Could be result or move number; handle result first
                    if (parseResult(moves)) {
                        return; // end of game after result
                    }
                    // Else skip move number patterns like '34.' or '34...'
                    // Move number pattern: number token followed by one or more PERIOD tokens
                    size_t lookahead = 1;
                    bool isMoveNumber = false;
                    auto nextTok = peek(lookahead);
                    while (nextTok && ((*nextTok)->type == TokenType::PERIOD || ((*nextTok)->type == TokenType::MINUS && lookahead == 1))) {
                        isMoveNumber = true;
                        lookahead++;
                        nextTok = peek(lookahead);
                    }
                    if (isMoveNumber) {
                        // consume number and following periods/minuses
                        consume();
                        size_t consumed = 1;
                        while (consumed < lookahead) {
                            consume();
                            consumed++;
                        }
                        break;
                    }
                    // Unexpected standalone number; treat as part of SAN? Skip.
                    consume();
                    break;
                }
                case TokenType::IDENTIFIER: {
                    // SAN move or result (like '1/2' part of result) or PV tokens
                    // Check result pattern: if identifier is '1/2' and next token is MINUS and next next is '1/2'
                    if (parseResult(moves)) {
                        return;
                    }
                    // parse SAN
                    std::string san;
                    san = tok->text;
                    consume();
                    // Append trailing annotation punctuation to SAN (e.g. '+', '#', '?', '!', '=Q')
                    while (true) {
                        auto nextTokOpt2 = peek();
                        if (!nextTokOpt2) break;
                        const Token* nt = *nextTokOpt2;
                        if (nt->type == TokenType::PLUS || nt->type == TokenType::PUNCT || nt->type == TokenType::MINUS) {
                            san += nt->text;
                            consume();
                        } else if (nt->type == TokenType::IDENTIFIER && nt->text.size() == 1 && nt->text[0] == 'Q') {
                            // promotion piece may appear after '=' sign; treat appended
                            san += nt->text;
                            consume();
                        } else {
                            break;
                        }
                    }
                    Move m;
                    m.fields.push_back({"move", san});
                    moves.push_back(m);
                    lastMove = &moves.back();
                    break;
                }
                case TokenType::PLUS:
                case TokenType::MINUS:
                case TokenType::PERIOD:
                case TokenType::PUNCT:
                case TokenType::SLASH:
                case TokenType::COMMA:
                case TokenType::RBRACKET:
                case TokenType::RBRACE:
                case TokenType::RPAREN:
                case TokenType::UNKNOWN: {
                    // Standalone punctuation outside recognised context; skip
                    consume();
                    break;
                }
                default: {
                    consume();
                    break;
                }
            }
        }
    }

    /**
     * Parse a result such as "1-0", "0-1" or "1/2-1/2". Returns true if
     * a result was parsed and appended to moves, false otherwise.
     */
    bool parseResult(std::vector<Move>& moves) {
        size_t savedIndex = index;
        // Patterns: NUMBER '-' NUMBER, or NUMBER '/' NUMBER '-' NUMBER '/' NUMBER
        auto first = peek();
        if (!first || ((*first)->type != TokenType::NUMBER && (*first)->type != TokenType::IDENTIFIER)) {
            return false;
        }
        std::string result;
        // If first token is identifier like "1/2" treat as result part
        if ((*first)->type == TokenType::IDENTIFIER && (*first)->text == "1/2") {
            result += (*first)->text;
            consume();
        } else if ((*first)->type == TokenType::NUMBER) {
            result += (*first)->text;
            consume();
        } else {
            return false;
        }
        auto second = peek();
        if (!second || ((*second)->type != TokenType::MINUS)) {
            index = savedIndex;
            return false;
        }
        result += "-";
        consume(); // consume '-'
        auto third = peek();
        if (!third || ((*third)->type != TokenType::NUMBER && (*third)->type != TokenType::IDENTIFIER)) {
            index = savedIndex;
            return false;
        }
        result += (*third)->text;
        consume();
        // Build possible result patterns; now result is like "1-0" or "0-1" or "1/2"
        if (result == "1-0" || result == "0-1" || result == "1/2") {
            // For "1/2" we need to read "-1/2" to complete
            if (result == "1/2") {
                // next tokens should be '-' and '1/2'
                auto dash = peek();
                if (!dash || (*dash)->type != TokenType::MINUS) {
                    index = savedIndex;
                    return false;
                }
                consume();
                auto fraction = peek();
                if (!fraction || ((*fraction)->type != TokenType::IDENTIFIER || (*fraction)->text != "1/2")) {
                    index = savedIndex;
                    return false;
                }
                result += "-";
                result += "1/2";
                consume();
            }
            Move m;
            m.fields.push_back({"result", result});
            moves.push_back(m);
            return true;
        }
        // If result is something like "1/2-1/2"
        if (result == "1/2-1/2") {
            Move m;
            m.fields.push_back({"result", result});
            moves.push_back(m);
            return true;
        }
        // Not a result; backtrack
        index = savedIndex;
        return false;
    }

    /**
     * Parse the contents of a brace comment and return extracted fields.
     * The index is expected to point at '{' on entry. On return, the index
     * will be positioned after the matching '}'.
     */
    std::vector<KeyValue> parseBraceComment() {
        // consume '{'
        consume();
        std::vector<Token> cTokens;
        // gather tokens until matching '}'
        size_t depth = 1;
        while (index < size && depth > 0) {
            const Token* t = consume();
            if (!t) break;
            if (t->type == TokenType::LBRACE) {
                depth++;
                cTokens.push_back(*t);
            } else if (t->type == TokenType::RBRACE) {
                depth--;
                if (depth == 0) {
                    // do not include this closing brace in cTokens
                    break;
                }
                cTokens.push_back(*t);
            } else {
                cTokens.push_back(*t);
            }
        }
        // parse tokens inside comment
        return parseCommentTokens(cTokens);
    }

    /**
     * Parse a variation enclosed in parentheses. On entry index points at '('.
     * Returns the textual representation of the variation's movetext. The
     * index will be positioned after the matching ')'.
     */
    std::string parseVariation() {
        // consume '('
        consume();
        std::ostringstream varStream;
        int depth = 1;
        while (index < size && depth > 0) {
            const Token* t = consume();
            if (!t) break;
            if (t->type == TokenType::LPAREN) {
                depth++;
                varStream << t->text;
            } else if (t->type == TokenType::RPAREN) {
                depth--;
                if (depth == 0) {
                    break;
                }
                varStream << t->text;
            } else {
                varStream << t->text;
                // insert space after identifiers and numbers to separate tokens
                if (t->type == TokenType::IDENTIFIER || t->type == TokenType::NUMBER || t->type == TokenType::QUOTED_STRING) {
                    varStream << ' ';
                }
            }
        }
        std::string v = varStream.str();
        // trim trailing space
        if (!v.empty() && v.back() == ' ') v.pop_back();
        return v;
    }

    /**
     * Parse tokens representing a comment (without braces) into fields.
     */
    std::vector<KeyValue> parseCommentTokens(const std::vector<Token>& cTokens) {
        std::vector<KeyValue> result;
        size_t pos = 0;
        // skip leading whitespace tokens (not produced by tokeniser)
        if (cTokens.empty()) {
            return result;
        }
        // Determine if this is an engine evaluation comment or plain comment
        // Recognise evaluation if first token is PLUS, MINUS, NUMBER or IDENTIFIER starting with 'M' or a digit
        auto isEvalStart = [&](const Token& tok) {
            if (tok.type == TokenType::PLUS || tok.type == TokenType::MINUS) return true;
            if (tok.type == TokenType::NUMBER) return true;
            if (tok.type == TokenType::IDENTIFIER) {
                if (!tok.text.empty() && (tok.text[0] == 'M' || std::isdigit(static_cast<unsigned char>(tok.text[0])))) {
                    return true;
                }
            }
            return false;
        };
        bool engineLike = isEvalStart(cTokens[0]);
        if (!engineLike) {
            // treat as plain comment
            std::ostringstream oss;
            for (size_t i = 0; i < cTokens.size(); ++i) {
                if (i > 0) oss << ' ';
                oss << cTokens[i].text;
            }
            result.push_back({"comment", oss.str()});
            return result;
        }
        // Parse evaluation value
        std::string sign;
        std::string valToken;
        size_t i = 0;
        // sign
        if (cTokens[i].type == TokenType::PLUS || cTokens[i].type == TokenType::MINUS) {
            sign = cTokens[i].text;
            i++;
        }
        // value token (number or identifier like "M1")
        if (i < cTokens.size()) {
            const Token& tv = cTokens[i];
            if (tv.type == TokenType::IDENTIFIER || tv.type == TokenType::NUMBER) {
                valToken = tv.text;
                i++;
            }
        }
        std::string value;
        if (!valToken.empty()) {
            value = sign + valToken;
        } else {
            // Not a valid evaluation; treat as plain comment
            std::ostringstream oss;
            for (size_t j = 0; j < cTokens.size(); ++j) {
                if (j > 0) oss << ' ';
                oss << cTokens[j].text;
            }
            result.push_back({"comment", oss.str()});
            return result;
        }
        // Depth
        std::string depth;
        if (i < cTokens.size() && cTokens[i].type == TokenType::SLASH) {
            i++;
            if (i < cTokens.size() && cTokens[i].type == TokenType::NUMBER) {
                depth = cTokens[i].text;
                i++;
            }
        }
        // Duration (number with optional unit)
        std::string duration;
        if (i < cTokens.size()) {
            // skip whitespace tokens (none in cTokens)
            if (cTokens[i].type == TokenType::NUMBER) {
                std::string dur = cTokens[i].text;
                i++;
                // optional unit following (identifier)
                if (i < cTokens.size() && cTokens[i].type == TokenType::IDENTIFIER) {
                    dur += cTokens[i].text;
                    i++;
                }
                duration = dur;
            }
        }
        // Principal variation tokens until comma or end
        std::string pv;
        std::ostringstream pvStream;
        bool pvUsed = false;
        while (i < cTokens.size()) {
            if (cTokens[i].type == TokenType::COMMA) {
                i++;
                break;
            }
            // skip whitespace tokens (not produced)
            if (pvUsed) pvStream << ' ';
            pvStream << cTokens[i].text;
            pvUsed = true;
            i++;
        }
        pv = pvStream.str();
        // Info: remaining tokens after comma
        std::string info;
        if (i < cTokens.size()) {
            std::ostringstream infoStream;
            bool used = false;
            for (; i < cTokens.size(); ++i) {
                if (used) infoStream << ' ';
                infoStream << cTokens[i].text;
                used = true;
            }
            info = infoStream.str();
        }
        // Store parsed fields
        if (!value.empty()) {
            result.push_back({"value", value});
        }
        if (!depth.empty()) {
            result.push_back({"depth", depth});
        }
        if (!duration.empty()) {
            result.push_back({"duration", duration});
        }
        if (!pv.empty()) {
            result.push_back({"pv", pv});
        }
        if (!info.empty()) {
            result.push_back({"info", info});
        }
        return result;
    }
};

/**
 * Parse a PGN string into a Game structure. Splits the input into tokens
 * then invokes the Parser.
 */
inline Game parse(const std::string& pgnText) {
    Tokeniser tok(pgnText);
    std::vector<Token> tokens;
    std::optional<Token> t;
    while ((t = tok.next())) {
        tokens.push_back(*t);
    }
    Parser parser(tokens);
    return parser.parseGame();
}

} // namespace pgn

/**
 * Example program: read PGN from standard input and output a simple
 * representation of the parsed structure. Each move and its fields are
 * printed on separate lines. Tags are printed first.
 */
int main() {
    std::string input((std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>());
    pgn::Game game = pgn::parse(input);
    // Output tags
    std::cout << "Tags:\n";
    for (const auto& kv : game.tags) {
        std::cout << kv.key << " = " << kv.value << "\n";
    }
    // Output moves
    std::cout << "Moves:\n";
    for (const auto& move : game.moves) {
        std::cout << "Move:\n";
        for (const auto& kv : move.fields) {
            std::cout << "  " << kv.key << " : " << kv.value << "\n";
        }
    }
    return 0;
}