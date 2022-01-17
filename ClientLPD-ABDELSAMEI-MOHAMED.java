//MOHAMED.ABDELSAMEI 133844 
import java.io.*;
import java.net.*;


public class ClientLPDConnreuse {
    public static void main(String[] argv) {

        if (argv.length != 2) {
            System.err.println("Uso corretto: java ClientLPDConnreuse server porta\n");
            System.exit(1);
        }

        try{
            BufferedReader fromUser = new BufferedReader(new InputStreamReader(System.in));

            Socket s = new Socket(argv[0], Integer.parseInt(argv[1]));
            InputStream fromServer = s.getInputStream();
            OutputStream toServer  = s.getOutputStream();

            do {
                System.out.println("Digita nome della linea produttiva di interesse ('fine' per terminare):");
                String nomelinea = fromUser.readLine();

                if (nomelinea.equals("fine")) {
                    break;
                }

                System.out.println("Digita il giorno di interesse in formato YYYYMMDD ('fine' per terminare):");
                String giorno = fromUser.readLine();

                if (giorno.equals("fine")) {
                    break;
                }

                System.out.println("Digita il numero N di fermi macchina di interesse ('fine' per terminare):");
                String N = fromUser.readLine();

                if (N.equals("fine")) {
                    break;
                }

                byte[] nomelinea_utf8 = nomelinea.getBytes("UTF-8");
                int nomelinea_len = nomelinea_utf8.length;
                byte[] len = new byte[2];
                len[0] = (byte)((nomelinea_len & 0xFF00) >> 8);
                len[1] = (byte)(nomelinea_len & 0xFF);

                toServer.write(len);
                toServer.write(nomelinea_utf8);

                byte[] giorno_utf8 = giorno.getBytes("UTF-8");
                int giorno_len = giorno_utf8.length;
                len[0] = (byte)((giorno_len & 0xFF00) >> 8);
                len[1] = (byte)(giorno_len & 0xFF);

                toServer.write(len);
                toServer.write(giorno_utf8);

                byte N_utf8[] = N.getBytes("UTF-8");
                int N_len = N_utf8.length;
                len[0] = (byte)((N_len & 0xFF00) >> 8);
                len[1] = (byte)(N_len & 0xFF);

                toServer.write(len);
                toServer.write(N_utf8);

                /* Leggo dimensione messaggio di risposta */
                fromServer.read(len);
                int to_read = ((int)(len[0] & 0xFF) << 8) | (int)(len[1] & 0xFF);

                /* Stampo contenuto risposta a video */
                while (to_read > 0) {
                    byte[] buffer = new byte[4096];

                    int bufsize = buffer.length;
                    int sz = (to_read < bufsize) ? to_read : bufsize;

                    int nread = fromServer.read(buffer, 0, sz);
                    System.out.write(buffer, 0, nread);

                    to_read -= nread;
                }

            } while(true);

            s.close();
        }
        catch(IOException e){
            System.err.println(e.getMessage());
            e.printStackTrace();
            System.exit(100);
        }
    }
}
