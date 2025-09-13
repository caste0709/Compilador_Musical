#include <iostream>
#include <fstream>
#include <regex>
#include <string>
#include <vector>
#include <map>
#include <iomanip>
#include "Mid/Lib.hpp" // Biblioteca MIDI
using namespace std;
using namespace MID;

struct Token {
    string tipo;
    string lexema;
    int linea;
};

vector<Token> todosLosTokens; // Global temporal

// Conversión nota a pitch MIDI (simplificado)
int notaAPitch(const string& nota, int octava) {
    map<string, int> base = {
        {"do", 0}, {"do#", 1}, {"re", 2}, {"re#", 3},
        {"mi", 4}, {"fa", 5}, {"fa#", 6}, {"sol", 7},
        {"sol#", 8}, {"la", 9}, {"la#", 10}, {"si", 11}
    };
    string notaBase = nota;
    if (nota.find('$') != string::npos) notaBase = nota.substr(0, nota.find('$')) + "#";
    return 12 * (octava + 1) + base[notaBase];
}

// Conversión figura a duración
double figuraADuracion(const string& figura) {
    if (figura == "r") return 4.0;
    if (figura == "b") return 2.0;
    if (figura == "n") return 1.0;
    if (figura == "c") return 0.5;
    if (figura == "sc") return 0.25;
    if (figura == "f") return 0.125;
    if (figura == "sf") return 0.0625;
    return 1;
}

class Parser {
    const vector<Token>& tokens;
    size_t pos;
    int octavaActual = 4;
    vector<tuple<double,double,int>> notasExtraidas; // t, dur, pitch
    double tiempoActual = 0;
    map<string, vector<Token>> patronesDefinidos;

public:
    Parser(const vector<Token>& tokens) : tokens(tokens), pos(0) {}
    
    const vector<tuple<double,double,int>>& obtenerNotas() const {
        return notasExtraidas;
    }

    const map<string, vector<Token>& getPatronesDefinidos() const {
        return patronesDefinidos;
    }
    void parse() {
        while (pos < tokens.size()) {
            if (tokens[pos].tipo == "Tempo" || tokens[pos].tipo == "Compas" ||
                tokens[pos].tipo == "Pulso" || tokens[pos].tipo == "Octava" ||
                tokens[pos].tipo == "Instrumento") { 
                if (tokens[pos].tipo == "Octava") {
                    string lex = tokens[pos].lexema;
                    octavaActual = stoi(lex.substr(4)); // oct=4
                }
                match(tokens[pos].tipo);
            } else if (tokens[pos].tipo == "Definicion") {
                string nombre = tokens[pos].lexema;
                nombre.pop_back();
                match("Definicion");
                size_t inicioBloque = pos;
                bloque();
                size_t finBloque = pos;
                patronesDefinidos[nombre] = vector<Token>(tokens.begin() + inicioBloque, tokens.begin() + finBloque);
            } else if (tokens[pos].tipo == "Repeat") {
                usos();
            } else {
                lanzarError("Tokens inesperados al final del archivo");
            }
        }
    }

    void parseBloqueSolo() {
        bloque();
    }

private:
    bool match(const string& tipo) {
        if (pos < tokens.size() && tokens[pos].tipo == tipo) {
            ++pos;
            return true;
        }
        return false;
    }

    const Token& current() const {
        if (pos >= tokens.size()) throw runtime_error("Fin inesperado de tokens");
        return tokens[pos];
    }

    void expect(const string& tipo) {
        if (!match(tipo)) {
            lanzarError("Se esperaba token de tipo '" + tipo + "' pero se encontro '" + current().tipo + "'");
        }
    }

    void lanzarError(const string& mensaje) const {
        const Token& t = current();
        throw runtime_error("Linea " + to_string(t.linea) + ": " + mensaje + " (lexema: '" + t.lexema + "')");
    }

    void declaraciones() {
        while (pos < tokens.size()) {
            if (tokens[pos].tipo == "Octava") {
                string lex = tokens[pos].lexema;
                octavaActual = stoi(lex.substr(4)); // oct=4
            }
            if (!match("Tempo") && !match("Compas") && !match("Pulso") && !match("Octava")) break;
        }
    }

    void patrones() {
        while (pos < tokens.size() && tokens[pos].tipo == "Definicion") {
            string nombre = tokens[pos].lexema;
            nombre.pop_back(); // quitar ':'
            match("Definicion");
            size_t inicioBloque = pos;
            bloque();
            size_t finBloque = pos;
            // Guardar tokens del bloque
            patronesDefinidos[nombre] = vector<Token>(tokens.begin() + inicioBloque, tokens.begin() + finBloque);
        }
    }

    void usos() {
        while (pos < tokens.size() && tokens[pos].tipo == "Repeat"){
            int lex = tokens[pos].lexema; // repeat(2) ritmo1
            smatch m;
            regex r("repeat\\((\\d+)\\) ([a-zA-Z_][a-zA-Z0-9_]*)");
            if (regex_match(lex, m, r)) {
                int rep = stoi(m[1]);
                string nombre = m[2];
                if (patronesDefinidos.count(nombre) == 0) {
                    lanzarError("Patron no definido: " + nombre);
                }
                for (int i = 0; i < rep; ++i) {
                    // Procesar el bloque guardado
                    Parser subparser(patronesDefinidos[nombre]);
                    subparser.octavaActual = octavaActual;
                    subparser.tiempoActual = tiempoActual;
                    subparser.parseBloqueSolo();
                    auto notas = subparser.obtenerNotas();
                    notasExtraidas.insert(notasExtraidas.end(), notas.begin(), notas.end());
                    tiempoActual = subparser.tiempoActual;
                }
            }
            match("Repeat");
        }
    }

    void bloque() {
        expect("BloqueInicio");
        while (pos < tokens.size() && tokens[pos].tipo != "BloqueFin") {
            compasRitmico();
            match("SeparadorCompas");
            match("RepeticionInicio");
            match("RepeticionFin");
        }
        expect("BloqueFin");
    }

    void compasRitmico() {
        while (pos < tokens.size() && tokens[pos].tipo == "NotaFigura") {
            string nota = tokens[pos].lexema; // (do#,n)
            smatch m;
            regex r("\\(([^,]+),([^\\)]+)\\)");
            if (regex_match(nota, m, r)) {
                string nombre = m[1];      // puede ser "do", "fa#", "sile", etc.
                string figura = m[2];      // "n", "c", "sc", etc.
                double dur = figuraADuracion(figura);
                bool tienePuntillo = match("DecoradorPuntillo");
                if (tienePuntillo) dur *= 1.5;

                if (nombre == "sile") {
                    // No se inserta nota, pero sí se avanza el tiempo
                    tiempoActual += dur;
                } else {
                    int pitch = notaAPitch(nombre, octavaActual);
                    notasExtraidas.emplace_back(tiempoActual, dur, pitch);
                    tiempoActual += dur;
                }
                cout << "Nota: " << nombre << " t=" << tiempoActual << " dur=" << dur << endl;
            }
            match("NotaFigura");
            match("DecoradorPuntillo"); // opcional
        }
    }
};

vector<Token> analizarLinea(const string& linea, int numeroLinea) {
    vector<Token> tokens;

    vector<pair<string, regex>> patrones = {
        {"NotaFigura", regex("\\((do|re|mi|fa|sol|la|si|sile)(#|\\$)?,(r|b|n|c|sc|f|sf)\\)")},
        {"Tempo", regex("tempo=\\d+")},
        {"Compas", regex("compas \\d+/\\d+")},
        {"Pulso", regex("pulso=[rnbcscfsf]+")},
        {"Octava", regex("oct=\\d+")},
        {"RepeticionInicio", regex("\\|:")},
        {"RepeticionFin", regex(":\\|")},
        {"SeparadorCompas", regex("\\|")},
        {"BloqueInicio", regex("\\{")},
        {"BloqueFin", regex("\\}")},
        {"DecoradorPuntillo", regex("\\.")},
        {"Definicion", regex("[a-zA-Z_][a-zA-Z0-9_]*:")},
        {"UsoPatron", regex("use [a-zA-Z_][a-zA-Z0-9_]*")},
        {"Repeat", regex("repeat\\(\\d+\\) [a-zA-Z_][a-zA-Z0-9_]*")},
        {"Instrumento", regex("instrumento\\s+[a-zA-Z_][a-zA-Z0-9_]*\\s*=\\s*\\d+")}
    };

    string resto = linea;
    while (!resto.empty()) {
        bool coincidencia = false;
        resto = regex_replace(resto, regex(R"(^\s+)"), "");

        for (const auto& [tipo, patron] : patrones) {
            smatch match;
            if (regex_search(resto, match, patron) && match.position() == 0) {
                tokens.push_back({tipo, match.str(), numeroLinea});
                resto = resto.substr(match.length());
                coincidencia = true;
                break;
            }
        }

        if (!coincidencia && !resto.empty()) {
            tokens.push_back({"Desconocido", string(1, resto[0]), numeroLinea});
            resto = resto.substr(1);
        }
    }

    return tokens;
}

int main() {
    ifstream archivo("entrada.txt");
    if (!archivo) {
        cerr << "No se pudo abrir el archivo.\n";
        return 1;
    }

    string linea;
    int numeroLinea = 1;
    vector<Token> todosLosTokens;
    while (getline(archivo, linea)) {
        vector<Token> lineaTokens = analizarLinea(linea, numeroLinea);
        todosLosTokens.insert(todosLosTokens.end(), lineaTokens.begin(), lineaTokens.end());
        ++numeroLinea;
    }

    ofstream archivoTokens("tokens.txt");
    archivoTokens << "Tipo\tLexema\n";
    for (const auto& token : todosLosTokens) {
        archivoTokens << token.tipo << "\t" << token.lexema << "\n";
    }
    archivoTokens.close();

    // Leer instrumentos definidos en la entrada
    map<string, int> instrumentosPorPatron;
    for (const auto& token : todosLosTokens) {
        if (token.tipo == "Instrumento") {
            smatch m;
            regex r("instrumento\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*=\\s*(\\d+)");
            if (regex_match(token.lexema, m, r)) {
                string nombre = m[1];
                int instrumento = stoi(m[2]);
                instrumentosPorPatron[nombre] = instrumento;
            }
        }
    }

    try {
        // 1. Parsear para obtener patrones definidos
        Parser parser(todosLosTokens);
        parser.parse();
        auto patronesDefinidos = parser.getPatronesDefinidos();

        // 2. Buscar todos los Repeat y procesar cada uno como pista independiente
        vector<vector<tuple<double,double,int>>> pistas;
        vector<string> nombresPatrones; // Guarda el nombre de cada patrón en el mismo orden que pistas
        for (size_t i = 0; i < todosLosTokens.size(); ++i) {
            if (todosLosTokens[i].tipo == "Repeat") {
                smatch m;
                regex r("repeat\\((\\d+)\\) ([a-zA-Z_][a-zA-Z0-9_]*)");
                if (regex_match(todosLosTokens[i].lexema, m, r)) {
                    int rep = stoi(m[1]);
                    string nombre = m[2];
                    if (patronesDefinidos.count(nombre)) {
                        Parser subparser(patronesDefinidos[nombre]);
                        subparser.parseBloqueSolo();
                        pistas.push_back(subparser.obtenerNotas());
                        nombresPatrones.push_back(nombre);
                    }
                }
            }
        }

        // 3. Crear el MIDI con varias pistas/canales y asignar instrumentos
        Sample sample(pistas.size());
        for (size_t i = 0; i < pistas.size(); ++i) {
            sample.currentTrack(i);
            int instrumento = 0;
            if (instrumentosPorPatron.count(nombresPatrones[i]))
                instrumento = instrumentosPorPatron[nombresPatrones[i]];
            sample << Instrument(0, i)(instrumento); // <-- Cambia el instrumento aquí
            for (auto [t, dur, pitch] : pistas[i]) {
                sample << Note(t, dur, pitch).channel(i); // Asegúrate de asignar el canal también
            }
        }
        sample.save("salida.mid");
        cout << "El archivo MIDI se genero correctamente como 'salida.mid'.\n";

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
    }

    return 0;
}