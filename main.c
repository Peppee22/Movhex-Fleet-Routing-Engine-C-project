#include <stdio.h> 
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define NUM_MAX_ROTTE_AEREE 5 /*Da specifica al massim0 5 rotte aeree concesse*/
#define NUM_VICINI 6 
static const int OFFSET_Y_PARI_SULLA_COL[NUM_VICINI] = {0, 1, 0, -1, -1, -1};/*In ordine: Nord-Est, Est, Sud-Est, Sud-Ovest, Ovest, Nord-Ovest sistemare*/
static const int OFFSET_Y_PARI_SULLA_RIG[NUM_VICINI] = {1, 0, -1, -1, 0, +1};
static const int OFFSET_Y_DISPARI_SULLA_COL[NUM_VICINI] = {1, 1, 1, 0, -1, 0};/*In ordine: Nord-Est, Est, Sud-Est, Sud-Ovest, Ovest, Nord-Ovest*/
static const int OFFSET_Y_DISPARI_SULLA_RIG[NUM_VICINI] = {1, 0, -1, -1, 0, +1};
#define CANCELLAZIONE 2000000000/*Per non dare problemi quando "cancello" (cosa che non posso fare perchè uso un array per le tratte) una tratta*/
#define BASE_COSTO_USCITA 1
#define BASE_NUMERO_TRATTE 0
#define PRIMO_STIMATO 100003/*Stimo che circa 100000 celle subiscano change e/o toggle*/
#define DIM_ARRAY_ULTIME_RICHIESTE 100

typedef struct 
  {
    int destinazione;
    int costo_tratta;
  }Tratta;

typedef struct Cella 
  {
    int posizione;
    int costo_uscita;
    int num_tratte;
    Tratta *tratte;
    struct Cella* next;
  }Cella;

typedef struct Richiesta{
  int pos_partenza;
  int pos_arrivo;
  int costo_calcolato;
}Richiesta;  

typedef struct Memoria{
  Richiesta Richieste[DIM_ARRAY_ULTIME_RICHIESTE];
  int num_richieste;
  int libero;
}Memoria;

typedef struct{
  int posizione;/*Nella mappa: col * m_rows + rig*/
  int dist; /*Distanza provvisoria*/
}Prop;

typedef struct{
  int heapsize;/*Dimensione attuale del min-heap*/
  int lenght; /*Dimensione dell'array con cui realizzo il min-heap*/
  Prop *Dati;/*Array che contiene, per ogni cella della mappa esagonale, la distanza dal sorgente e l'indice dell'esagono che stiamo analizzando*/
}Min_Heap;

int m_rows = 0;
int m_cols = 0;
int n_elementi = 0;
int num_chiamate_init = 0;
int Tab_Hash_size = 0;
Cella** Mappa_Hash = NULL;
Min_Heap* Heap = NULL;
Prop *Dati_glob = NULL;  
int *distanze = NULL;
bool *visitati = NULL;
Memoria memoria;
int* da_azzerare = NULL;
int dim_da_azzerare = 0;

void init(int n_colonne, int n_righe);
int Funzione_Hashing(int pos);
void Rimuovi_Cella(int pos);
Cella* Cerca_Cella(int pos);
int DistEsagoni(int xa, int ya, int xb, int yb);
void change_cost(int x, int y, int v, int raggio);
void toggle_air_route(int x1, int y1, int x2, int y2);
int travel_cost(int xp, int yp, int xd, int yd);
void Inserisci_Min_Heap(int pos_es, int distanza);
Prop Heap_Pop();
void Cancella_Memoria();
void Inserisci_in_memoria(int pos_partenza, int pos_arrivo, int costo);
int Cerca_Memoria(int pos_partenza, int pos_arrivo);
void Resetta_Array_Heap();

int main(){
  int col, rig, col_1, rig_1, ris;  
  char nome_funzione[17]; /*16 dimensione massima del nome delle procedure/funzioni da eseguire*/
  while(scanf("%16s", nome_funzione) != EOF){
    if(!strcmp(nome_funzione, "init")){
      ris = scanf("%d %d", &col, &rig);
      init(col, rig);
    }else if(!strcmp(nome_funzione, "change_cost")){
      ris = scanf("%d %d %d %d", &col, &rig, &col_1, &rig_1);
      change_cost(col, rig, col_1, rig_1);
    }else if(!strcmp(nome_funzione, "toggle_air_route")){
      ris = scanf("%d %d %d %d", &col, &rig, &col_1, &rig_1);
      toggle_air_route(col, rig, col_1, rig_1);
    }else{
      ris = scanf("%d %d %d %d", &col, &rig, &col_1, &rig_1);
      ris = travel_cost(col, rig, col_1, rig_1);
      printf("%d\n", ris);
    }
  }
  ris = ris - ris;
  return 0;
}

void Cancella_Memoria(){
  memoria.num_richieste = 0;
  memoria.libero = 0;
}

void Inserisci_in_memoria(int pos_partenza, int pos_arrivo, int costo){
  int i;

  i = memoria.libero;

  memoria.Richieste[i].pos_partenza = pos_partenza;
  memoria.Richieste[i].pos_arrivo = pos_arrivo;
  memoria.Richieste[i].costo_calcolato = costo;

  memoria.libero = (memoria.num_richieste + 1) % DIM_ARRAY_ULTIME_RICHIESTE;
  if(memoria.num_richieste < DIM_ARRAY_ULTIME_RICHIESTE){
    memoria.num_richieste++;
  }
}

int Cerca_Memoria(int pos_partenza, int pos_arrivo){
  int i = 0;

  for(i = 0; i < memoria.num_richieste; i++){
    if((memoria.Richieste[i].pos_partenza == pos_partenza) && (memoria.Richieste[i].pos_arrivo == pos_arrivo)){
      return memoria.Richieste[i].costo_calcolato;
    }
  }

  return 0;

}

int Funzione_Hashing(int pos){/*Passare sempre a Mappa_Hash Funzione_Hashing(pos)*/
  if(Tab_Hash_size <= 0){
    return 0;
  }
  return pos % Tab_Hash_size;
}

Cella* Cerca_Cella(int pos){/*restituisce il puntatore alla cella cercata*/
  int pos_analisi;
  Cella* in_analisi;

  pos_analisi = Funzione_Hashing(pos);
  in_analisi = Mappa_Hash[pos_analisi];

  while(in_analisi){
    if(in_analisi->posizione == pos){
      return (in_analisi);
    }
    in_analisi = in_analisi->next;
  }

  return NULL;
}

void Rimuovi_Cella(int pos){/*Se torno alle impostazioni "base" elimino*/
  int pos_analisi, trovato;
  Cella* in_analisi, *prev;

  pos_analisi = Funzione_Hashing(pos);
  in_analisi = Mappa_Hash[pos_analisi];
  prev = NULL;
  trovato = 0;

  while(in_analisi && !trovato){/*Se esiste la cella: la cella di Hash non mi punta a NULL*/
    if(in_analisi->posizione == pos){/*Scorro nella lista fino a quella cercata*/
      if(prev){/*Se non sono al pirmo argomento della lista*/
        prev->next = in_analisi->next;
      }else{/*Aggiorno "testa"*/
        Mappa_Hash[pos_analisi] = in_analisi->next;
      }
      if(in_analisi->tratte){/*Se ho tratte, le cancello*/
        free(in_analisi->tratte);
      }
      trovato = 1;
    }
    if(!trovato){
      prev = in_analisi;
      in_analisi = in_analisi->next;  
    }
  }
  if(trovato){
    free(in_analisi);
  }
}

void Resetta_Array_Heap(){
  int i;

  for(i = 0; i < dim_da_azzerare; i++){
    distanze[da_azzerare[i]] = CANCELLAZIONE;
    visitati[da_azzerare[i]] = 0;
  }
  dim_da_azzerare = 0;
}

void init(int n_colonne, int n_righe){
  int i;
  Cella* in_analisi, *next;

  if(num_chiamate_init){/*Mappa già inizializzata*/
    for(i = 0; i < Tab_Hash_size; i++){
      in_analisi = Mappa_Hash[i];
      while(in_analisi){
        next = in_analisi->next;
        if(in_analisi->tratte){/*Se ho l'array di tratte cancello*/
          free(in_analisi->tratte);
        }
        free(in_analisi);
        in_analisi = next;
      }
    }
    free(Mappa_Hash);
    free(visitati);
    free(distanze);
    free(Dati_glob);
    free(Heap);
    free(da_azzerare);
    Cancella_Memoria();
  }

  m_rows = n_righe;
  m_cols = n_colonne;
  num_chiamate_init++;
  n_elementi = m_cols * m_rows;
  Tab_Hash_size = PRIMO_STIMATO;
  Mappa_Hash = calloc(Tab_Hash_size, sizeof(Cella*));

  /*Inizializzo cose per Dijkstra: travel cost*/
  Heap = malloc(sizeof(Min_Heap));  
  Dati_glob = malloc(n_elementi * sizeof(Prop));
  Heap->Dati = Dati_glob;
  Heap->lenght = n_elementi;
  Heap->heapsize = 0;

  visitati = malloc(n_elementi * sizeof(bool));
  distanze = malloc(n_elementi * sizeof(int));
  for(i = 0; i < n_elementi; i++){
    visitati[i] = 0;
    distanze[i] = CANCELLAZIONE;
  }
  da_azzerare = malloc(n_elementi * sizeof(int));
  dim_da_azzerare = 0;
  Cancella_Memoria();           
  printf("OK\n");
}


void Inserisci_Min_Heap(int pos_es, int distanza){/*Cosa inserisco quando creo un elemento nel min-heap: una coppia pos esagono e distanza*/
  int i, padre;
  Prop temp;

  if(Heap->heapsize < Heap->lenght){
    i = Heap->heapsize;

    /*Inserisco in coda nel min-heap*/
    Heap->Dati[Heap->heapsize].posizione = pos_es;
    Heap->Dati[Heap->heapsize].dist = distanza;
    /*Devo ricostruire la struttura del min-heap con l'invariante: la chiave del padre è minore di quella del figlio*/
    padre = (i - 1) / 2;
    while((i > 0) && (Heap->Dati[padre].dist > Heap->Dati[i].dist)){
      /*Swap padre/figlio*/
      temp = Heap->Dati[padre];
      Heap->Dati[padre] = Heap->Dati[i];
      Heap->Dati[i] = temp;
      i = padre;
      padre = (i - 1) / 2;
    }
    Heap->heapsize++;
  }
}

Prop Heap_Pop(){/*Restituisce l'elemento con la distanza minima*/
  int i, figlio_sx, figlio_dx, minore;
  Prop Min, temp;

  Min = Heap->Dati[0];/*Salvo il minimo da restituire*/
  Heap->Dati[0] = Heap->Dati[Heap->heapsize - 1];/*Sposta l'ultimo elemento nella radice*/
  Heap->heapsize--;/*Diminuisco la dimensione del min-heap: ricorda che rimane il dato nella posizione successiva: sto usando un'array fisso*/
  i = 0;
  figlio_sx = 2 * i + 1;

  figlio_dx = figlio_sx + 1;
  while((figlio_sx < Heap->heapsize) && (Heap->Dati[figlio_sx].dist < Heap->Dati[i].dist || (figlio_dx < Heap->heapsize && Heap->Dati[figlio_dx].dist < Heap->Dati[i].dist))){
    /*Scelgo il più piccolo*/
    if((figlio_dx < Heap->heapsize) && (Heap->Dati[figlio_dx].dist <= Heap->Dati[figlio_sx].dist)){
      minore = figlio_dx;  
    }else{
      minore = figlio_sx;
    }
    temp = Heap->Dati[i];
    Heap->Dati[i] = Heap->Dati[minore];
    Heap->Dati[minore] = temp;

    i = minore;
    figlio_sx = 2 * i + 1;/*Aggiorno figlio_sx*/
    figlio_dx = figlio_sx + 1;/*Aggiorno figlio_dx*/
  }

  return Min;
}

int travel_cost(int xp, int yp, int xd, int yd){
  int somma, i, pos_partenza, pos_destinazione, pos_da_anal, pos_analisi, x_pos_analisi, y_pos_analisi, x_pos_da_anal, y_pos_da_anal;
  int costo_uscita, num_tratte;
  const int* offset_col, *offset_rig;
  int ris_memoria;
  Prop analizzato;
  Cella* in_analisi;

  if(!num_chiamate_init){
    return -1;
  }

  if((xp < 0 || xp >= m_cols) || (yp < 0 || yp >= m_rows) || (xd < 0 || xd >= m_cols) || (yd < 0 || yd >= m_rows)){
    return -1;
  }

  if((xp == xd) && (yp == yd)){
    return 0;
  }
  
  pos_partenza = xp * m_rows + yp;
  pos_destinazione = xd * m_rows + yd;
  ris_memoria = Cerca_Memoria(pos_partenza, pos_destinazione);
  if(ris_memoria){
    return ris_memoria;
  }

  /*Reset Memoria*/
  Resetta_Array_Heap();

  Heap->heapsize = 0;
  distanze[pos_partenza] = 0;
  da_azzerare[dim_da_azzerare] = pos_partenza;
  dim_da_azzerare++; 
  Inserisci_Min_Heap(pos_partenza, 0);

  /*Inizio loop Dijkstra*/
  while(Heap->heapsize > 0){
    analizzato = Heap_Pop();
    pos_analisi = analizzato.posizione;

    if((!visitati[pos_analisi]) && (analizzato.dist == distanze[pos_analisi])){/*Se non l'ho già poppato e se non è un entry obsoleta da lazy dec*/
      visitati[pos_analisi] = 1;
      if(pos_analisi == pos_destinazione){
        Inserisci_in_memoria(pos_partenza, pos_destinazione, distanze[pos_analisi]);
        return distanze[pos_analisi];
      }

      in_analisi = Cerca_Cella(pos_analisi);
      if(in_analisi){
        costo_uscita = in_analisi->costo_uscita;
        num_tratte = in_analisi->num_tratte;
      }else{
        costo_uscita = BASE_COSTO_USCITA;
        num_tratte = BASE_NUMERO_TRATTE;
      } 

      x_pos_analisi = pos_analisi / m_rows;
      y_pos_analisi = pos_analisi % m_rows;
      if((y_pos_analisi % 2) == 0){/*riga dell'esagono che sto analizzando pari*/
        offset_col = OFFSET_Y_PARI_SULLA_COL;
        offset_rig = OFFSET_Y_PARI_SULLA_RIG;
      }else{
        offset_col = OFFSET_Y_DISPARI_SULLA_COL;
        offset_rig = OFFSET_Y_DISPARI_SULLA_RIG;
      }        

      /*Costi di uscita via terra*/
      if(costo_uscita > 0){/*Se il costo di uscita NON è 0 (intransitabile) e non sono arrivato alla destinazione*/
        for(i = 0; i < NUM_VICINI; i++){/*Scorro tutti i vicini*/
          x_pos_da_anal = x_pos_analisi + offset_col[i];
          y_pos_da_anal = y_pos_analisi + offset_rig[i];
          if(x_pos_da_anal >= 0 && x_pos_da_anal < m_cols && y_pos_da_anal >= 0 && y_pos_da_anal < m_rows){
            pos_da_anal = x_pos_da_anal * m_rows + y_pos_da_anal;
            if(!visitati[pos_da_anal]){/*Sono nei margini e non posto visitato a 1 per l'esagono*/
              somma = distanze[pos_analisi] + costo_uscita;
              if(somma < distanze[pos_da_anal]){
                if(distanze[pos_da_anal] == CANCELLAZIONE){
                  da_azzerare[dim_da_azzerare] = pos_da_anal;
                  dim_da_azzerare++;
                }
                distanze[pos_da_anal] = somma;
                Inserisci_Min_Heap(pos_da_anal, somma);
              }
            }
          }  
        }
      }/*Fine costi via terra*/

      /*Costo delle varie tratte*/
      if((in_analisi) && (num_tratte != 0)){
        for(i = 0; i < num_tratte; i++){
          if(in_analisi->tratte[i].costo_tratta > 0){
            pos_da_anal = in_analisi->tratte[i].destinazione;/*Esagono di arrivo della tratta*/
            if(!visitati[pos_da_anal]){  
              somma = distanze[pos_analisi] + in_analisi->tratte[i].costo_tratta;
              if(somma < distanze[pos_da_anal]){
                if(distanze[pos_da_anal] == CANCELLAZIONE){
                  da_azzerare[dim_da_azzerare] = pos_da_anal;
                  dim_da_azzerare++;
                }
                distanze[pos_da_anal] = somma;
                Inserisci_Min_Heap(pos_da_anal, somma);
              }
            }
          }
        }
      }
    }
  }
  Inserisci_in_memoria(pos_partenza, pos_destinazione, -1);
  return -1;
}

/*Ogni rotta aerea è identificata univocamente dalla coppia ordinata dei due esagoni */
void toggle_air_route(int x1, int y1, int x2, int y2){/*(x1, y1) esagono di partenza e (x2, y2) esagono di arrivo*/
  int i, pos_ins, pos_dest, pos_canc, pos_analisi;
  bool trovato;

  if(num_chiamate_init){
    if((x1 >= 0 && x1 < m_cols) && (y1 >= 0 && y1 < m_rows) && (x2 >= 0 && x2 < m_cols) && (y2 >= 0 && y2 < m_rows)){
      trovato = 0;
      pos_ins = x1 * m_rows + y1;
      pos_dest = x2 * m_rows + y2;
      Cella* in_analisi;
      pos_analisi = Funzione_Hashing(pos_ins);

      in_analisi = Cerca_Cella(pos_ins);/*Cerco se già ho la cella nella mappa*/
      if(!in_analisi){/*Se non c'è, la devo creare*/
        in_analisi = malloc(sizeof(Cella));
        in_analisi->posizione = pos_ins;
        in_analisi->costo_uscita = BASE_COSTO_USCITA;
        in_analisi->num_tratte = 0;
        in_analisi->tratte = NULL;
        in_analisi->next = Mappa_Hash[pos_analisi];
        Mappa_Hash[pos_analisi] = in_analisi;
      }
      if(!in_analisi->tratte){/*Se non esiste l'array di tratte lo alloco*/
        in_analisi->tratte = malloc(NUM_MAX_ROTTE_AEREE * sizeof(Tratta));
      }
      /*Scorro l'array di tratte, se trovo già la tratta, allora la devo cancellare*/
      for(i = 0; i < in_analisi->num_tratte && !trovato; i++){
        if(in_analisi->tratte[i].destinazione == pos_dest){/*La tratta già esiste, devo: Cancellare la tratta spostando tuttoo l'array e diminuire il numero di tratte dell'esagono di partenza*/
          for(pos_canc = i; pos_canc < in_analisi->num_tratte -1; pos_canc++){
            in_analisi->tratte[pos_canc] = in_analisi->tratte[pos_canc + 1]; 
          }
          in_analisi->num_tratte--;
          Cancella_Memoria();
          printf("OK\n");

          /*Devo togliere dalla hash se costo = 1 e 0 tratte*/
          if((in_analisi->costo_uscita == BASE_COSTO_USCITA) && (in_analisi->num_tratte == 0)){/*Tolgo dalla hashmap perchè tornato base*/
            Rimuovi_Cella(pos_ins);
          }
          trovato = 1;
        }
      }

      /*Se non l'ho trovata la devo aggiungere sempre se ho meno di 5 tratte*/
      if(!trovato && in_analisi->num_tratte < NUM_MAX_ROTTE_AEREE){
        in_analisi->tratte[in_analisi->num_tratte].destinazione = pos_dest;
        in_analisi->tratte[in_analisi->num_tratte].costo_tratta = in_analisi->costo_uscita;
        in_analisi->num_tratte++;
        Cancella_Memoria();
        printf("OK\n");
      }
    }else{
      printf("KO\n");
    }
  }else{
    printf("KO\n");
  }
}
/*Idea: Parto dall'esagono (x - rggio, y- raggio) e costruisco un quadrato fino a (x + raggio, y + raggio) controllando per ogni esagono se la distanza è corretta.
  Il -1 serve perché la specifica dice così*/
void change_cost(int x, int y, int v, int raggio){
  int i, j, tratta, dist, somma, pos, num_tratte;
  float peso;
  Cella* in_analisi;

  if((x >= 0 && x < m_cols) && (y >= 0 && y < m_rows) && (raggio > 0) && ((v >= -10) && (v <= 10)) && (num_chiamate_init)){
    for(i = x - raggio + 1; i <= x + raggio -1; i++){
      for(j = y - raggio + 1; j <= y + raggio - 1; j++){
        if(!((i < 0 || i >= m_cols) || (j < 0 || j >= m_rows))){
          dist = DistEsagoni(i, j, x, y);
          if(dist < raggio){/*esagono che ha coordinate i, j e esagono in posizione x y*/
            peso = (float)(raggio - dist) / (float)(raggio);
            if(peso < 0){
              peso = 0;
            }
            pos = i * m_rows + j;
            in_analisi = Cerca_Cella(pos);/*Se non esiste la cella la creo*/

            if(in_analisi){/*Se la cella esiste, calcolo il nuovo costo con il valore che aveva*/
              somma = in_analisi->costo_uscita + (int)floor(v * peso);/*Modifico il peso dell'uscita dell'esagono*/
              num_tratte = in_analisi->num_tratte;
              if(somma < 0){
                somma = 0;
              }
              if(somma > 100){
                somma = 100;
              }

              if(somma == 1){
                if(num_tratte == 0){
                  Rimuovi_Cella(pos);  
                }else{/*Somma = 1 ma ho delle tratte, non posso cancellare; quindi aggiorno tutte le tratte per avere costo 1*/
                  in_analisi->costo_uscita = somma;
                  for(tratta = 0; tratta < num_tratte; tratta++){
                    in_analisi->tratte[tratta].costo_tratta = somma;/*Modifico il peso di tutte le tratte dell'esagono*/
                  }  
                }
              }else{/*Il costo non è 1, aggiorno tutto normalmente*/
                in_analisi->costo_uscita = somma;
                  for(tratta = 0; tratta < num_tratte; tratta++){
                    in_analisi->tratte[tratta].costo_tratta = somma;/*Modifico il peso di tutte le tratte dell'esagono*/
                  }
              }
            }else{/*La cella non esiste: la creo; nota: se non esiste, allora non ha sicuramente tratte, quindi non le devo aggiornare*/
              somma = BASE_COSTO_USCITA + (int)floor(v * peso);/*Modifico il peso dell'uscita dell'esagono*/
              if(somma < 0){
                somma = 0;
              }
              if(somma > 100){
                somma = 100;
              }
              if(somma != 1){/*Se non esiste, non ha nemmeno tratte, quindi se ha nuovo_costo 1 non metto niente nella Hash*/
                in_analisi = malloc(sizeof(Cella));
                if(in_analisi){
                  in_analisi->posizione = pos;
                  in_analisi->costo_uscita = somma;
                  in_analisi->num_tratte = 0;
                  in_analisi->tratte = NULL;
                  /*Inserisco in testa alla coda nella cella giusta*/
                  in_analisi->next = Mappa_Hash[Funzione_Hashing(pos)];
                  Mappa_Hash[Funzione_Hashing(pos)] = in_analisi;
                }  
              }
            }
          }
        }
      }
    } 
    Cancella_Memoria();
    printf("OK\n");
  }else{
    printf("KO\n");
  }
}


int DistEsagoni(int xa, int ya, int xb, int yb){
  int ax_xa_1, ax_xb_1, rag_ya, rag_yb, cub_x_1, cub_y_1, cub_z_1, cub_x_2, cub_y_2, cub_z_2;
  int dist_x, dist_y, dist_z, dist_tot;

  /*Passaggio ad assiali*/
  ax_xa_1 = xa - (ya / 2);
  rag_ya = ya;
  ax_xb_1 = xb - (yb / 2);
  rag_yb = yb;

  /*Passaggio da assiali a cubiche*/
  cub_x_1 = ax_xa_1;
  cub_z_1 = rag_ya;
  cub_y_1 = - cub_x_1 - cub_z_1;
  cub_x_2 = ax_xb_1;
  cub_z_2 = rag_yb;
  cub_y_2 = - cub_x_2 - cub_z_2;

  /*Calcolo distanza*/
  if(cub_x_1 > cub_x_2){
    dist_x = cub_x_1 - cub_x_2;
  }else{
    dist_x = cub_x_2 - cub_x_1;
  }
  if(cub_y_1 > cub_y_2){
    dist_y = cub_y_1 - cub_y_2;
  }else{
    dist_y = cub_y_2 - cub_y_1;
  }
  if(cub_z_1 > cub_z_2){
    dist_z = cub_z_1 - cub_z_2;
  }else{
    dist_z = cub_z_2 - cub_z_1;
  }

  dist_tot = dist_x;
  if(dist_y > dist_tot){
    dist_tot = dist_y;
  }
  if(dist_z > dist_tot){
    dist_tot = dist_z;
  }

  return dist_tot;
}


