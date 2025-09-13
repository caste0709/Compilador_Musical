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

    // Conversión nota a pitch MIDI (simplificado)
    static int notaAPitch(String nota, int octava) {
        Map<String, Integer> base = Map.ofEntries(
            Map.entry("do", 0), Map.entry("do#", 1), Map.entry("re", 2), Map.entry("re#", 3),
            Map.entry("mi", 4), Map.entry("fa", 5), Map.entry("fa#", 6), Map.entry("sol", 7),
            Map.entry("sol#", 8), Map.entry("la", 9), Map.entry("la#", 10), Map.entry("si", 11)
        );
        String notaBase = nota;
        if (nota.contains("$")) {
            notaBase = nota.substring(0, nota.indexOf('$')) + "#";
        }
        return 12 * (octava + 1) + base.getOrDefault(notaBase, 0);
    }

    // Conversión figura a duración
    static double figuraADuracion(String figura) {
        return switch (figura) {
            case "r" -> 4.0;
            case "b" -> 2.0;
            case "n" -> 1.0;
            case "c" -> 0.5;
            case "sc" -> 0.25;
            case "f" -> 0.125;
            case "sf" -> 0.0625;
            default -> 1.0;
        };
    }

    // Analizador de una línea
    static List<Token> analizarLinea(String linea, int numeroLinea) {
        List<Token> tokens = new ArrayList<>();

        Map<String, Pattern> patrones = Map.ofEntries(
            Map.entry("NotaFigura", Pattern.compile("\\((do|re|mi|fa|sol|la|si|sile)(#|\\$)?,(r|b|n|c|sc|f|sf)\\)")),
            Map.entry("Tempo", Pattern.compile("tempo=\\d+")),
            Map.entry("Compas", Pattern.compile("compas \\d+/\\d+")),
            Map.entry("Pulso", Pattern.compile("pulso=[rnbcscfsf]+")),
            Map.entry("Octava", Pattern.compile("oct=\\d+")),
            Map.entry("RepeticionInicio", Pattern.compile("\\|:")),
            Map.entry("RepeticionFin", Pattern.compile(":\\|")),
            Map.entry("SeparadorCompas", Pattern.compile("\\|")),
            Map.entry("BloqueInicio", Pattern.compile("\\{")),
            Map.entry("BloqueFin", Pattern.compile("\\}")),
            Map.entry("DecoradorPuntillo", Pattern.compile("\\.")),
            Map.entry("Definicion", Pattern.compile("[a-zA-Z_][a-zA-Z0-9_]*:")),
            Map.entry("UsoPatron", Pattern.compile("use [a-zA-Z_][a-zA-Z0-9_]*")),
            Map.entry("Repeat", Pattern.compile("repeat\\(\\d+\\) [a-zA-Z_][a-zA-Z0-9_]*")),
            Map.entry("Instrumento", Pattern.compile("instrumento\\s+[a-zA-Z_][a-zA-Z0-9_]*\\s*=\\s*\\d+"))
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

        // Leer entrada.txt
        try (BufferedReader br = new BufferedReader(new FileReader("entrada.txt"))) {
            String linea;
            int numeroLinea = 1;
            while ((linea = br.readLine()) != null) {
                todosLosTokens.addAll(analizarLinea(linea, numeroLinea));
                numeroLinea++;
            }
        } catch (IOException e) {
            System.err.println("No se pudo abrir el archivo.");
            return;
        }

        // Guardar tokens.txt
        try (PrintWriter pw = new PrintWriter(new FileWriter("tokens.txt"))) {
            pw.println("Tipo\tLexema");
            for (Token token : todosLosTokens) {
                pw.println(token.tipo + "\t" + token.lexema);
            }
        } catch (IOException e) {
            System.err.println("No se pudo escribir tokens.txt");
        }

        // Aquí podrías implementar la clase Parser en Java como en C++,
        // para manejar Definicion, Repeat, Bloques, etc.
        System.out.println("Se analizaron " + todosLosTokens.size() + " tokens.");
    }
}
