import java.io.*;
import java.util.*;
import java.util.regex.*;

class Token {
    String tipo;
    String lexema;
    int linea;

    Token(String tipo, String lexema, int linea) {
        this.tipo = tipo;
        this.lexema = lexema;
        this.linea = linea;
    }
}

public class Main {

    static int notaAPitch(String nota, int octava) {
        Map<String, Integer> base = Map.of(
            "do", 0, "do#", 1, "re", 2, "re#", 3,
            "mi", 4, "fa", 5, "fa#", 6, "sol", 7,
            "sol#", 8, "la", 9, "la#", 10, "si", 11
        );

        String notaBase = nota;
        if (nota.endsWith("$")) {
            notaBase = nota.substring(0, nota.length() - 1) + "#";
        }
        return 12 * (octava + 1) + base.getOrDefault(notaBase, 0);
    }

    static double figuraADuracion(String figura) {
        switch (figura) {
            case "r": return 4.0;
            case "b": return 2.0;
            case "n": return 1.0;
            case "c": return 0.5;
            case "sc": return 0.25;
            case "f": return 0.125;
            case "sf": return 0.0625;
            default: return 1.0;
        }
    }

    static List<Token> analizarLinea(String linea, int numeroLinea) {
        List<Token> tokens = new ArrayList<>();

        Map<String, Pattern> patrones = Map.of(
            "NotaFigura", Pattern.compile("\\((do|re|mi|fa|sol|la|si|sile)(#|\\$)?,(r|b|n|c|sc|f|sf)\\)"),
            "Tempo", Pattern.compile("tempo=\\d+"),
            "Compas", Pattern.compile("compas \\d+/\\d+"),
            "Pulso", Pattern.compile("pulso=[rnbcscfsf]+"),
            "Octava", Pattern.compile("oct=\\d+"),
            "RepeticionInicio", Pattern.compile("\\|:"),
            "RepeticionFin", Pattern.compile(":\\|"),
            "SeparadorCompas", Pattern.compile("\\|"),
            "BloqueInicio", Pattern.compile("\\{"),
            "BloqueFin", Pattern.compile("\\}"),
            "DecoradorPuntillo", Pattern.compile("\\."),
            "Definicion", Pattern.compile("[a-zA-Z_][a-zA-Z0-9_]*:"),
            "UsoPatron", Pattern.compile("use [a-zA-Z_][a-zA-Z0-9_]*"),
            "Repeat", Pattern.compile("repeat\\(\\d+\\) [a-zA-Z_][a-zA-Z0-9_]*"),
            "Instrumento", Pattern.compile("instrumento\\s+[a-zA-Z_][a-zA-Z0-9_]*\\s*=\\s*\\d+")
        );

        String resto = linea;
        while (!resto.isEmpty()) {
            boolean coincidencia = false;
            resto = resto.replaceFirst("^\\s+", "");

            for (var entry : patrones.entrySet()) {
                Matcher m = entry.getValue().matcher(resto);
                if (m.find() && m.start() == 0) {
                    tokens.add(new Token(entry.getKey(), m.group(), numeroLinea));
                    resto = resto.substring(m.end());
                    coincidencia = true;
                    break;
                }
            }

            if (!coincidencia && !resto.isEmpty()) {
                tokens.add(new Token("Desconocido", String.valueOf(resto.charAt(0)), numeroLinea));
                resto = resto.substring(1);
            }
        }

        return tokens;
    }

    public static void main(String[] args) {
        List<Token> todosLosTokens = new ArrayList<>();

        try (BufferedReader br = new BufferedReader(new FileReader("entrada.txt"))) {
            String linea;
            int numeroLinea = 1;
            while ((linea = br.readLine()) != null) {
                todosLosTokens.addAll(analizarLinea(linea, numeroLinea));
                numeroLinea++;
            }
        } catch (IOException e) {
            System.err.println("No se pudo abrir el archivo: " + e.getMessage());
            return;
        }

        try (PrintWriter pw = new PrintWriter(new FileWriter("tokens.txt"))) {
            pw.println("Tipo\tLexema");
            for (Token token : todosLosTokens) {
                pw.println(token.tipo + "\t" + token.lexema);
            }
        } catch (IOException e) {
            System.err.println("No se pudo escribir tokens.txt: " + e.getMessage());
        }

        System.out.println("Se analizaron " + todosLosTokens.size() + " tokens.");
    }
}

