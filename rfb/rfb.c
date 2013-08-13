/*
 * rfb.c
 *
 *
 */

#include "rfb.h"

/* Stuff for on_exit() */
/* FIXME: on_exit() is non-portable. Don't use it. */
void heap_release(int stat, void *ptr) {free(ptr);}

/* Print an error and die. */
void std_die(char *who, int stat)
{perror(who); exit(stat);}
void client_die(char *who, int stat)
{perror(who); pthread_exit(0); pthread_exit(NULL);}
void gai_die(char *who, int stat)
{fprintf(stderr, "%s: %s", who, gai_strerror(stat)); exit(stat);}

/* Initialize and return a socket. */
/* FIXME: Need to handle the case where getaddrinfo()
 * returns multiple addrinfo structs. Also, at the
 * moment, rfb_sockinit() never fails without crashing
 * the program. Is this a good idea?
 */
int rfb_sockinit(const char *serv)
{
  int err, sockfd;
  struct addrinfo hints;
  struct addrinfo *ai = NULL; /* addrinfo for the new socket */

  /* Hints for getaddrinfo() */
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC; /* IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM; /* tcp */
  hints.ai_flags = AI_PASSIVE; /* get node automagically */

  /* Get info for an RFB socket. */
  err = getaddrinfo((char*) NULL, serv, &hints, &ai);
  if(err) gai_die("getaddrinfo()", err);

  /* Get a host socket fd. */
  errno = 0;
  sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
  if(sockfd < 0) std_die("socket()", errno);

  /* Bind the socket to the appropriate sockaddr. */
  errno = 0;
  err = bind(sockfd, ai->ai_addr, ai->ai_addrlen);
  if(err) std_die("bind()", errno);

  freeaddrinfo(ai);

  return sockfd;
}

/* Send a complete stream with error checking. */
int sendall(int sockfd, const void *buf, size_t sz)
{
  int actual;

  while(sz > 0)
  {
    errno = 0;
    actual = send(sockfd, buf, sz, MSG_NOSIGNAL);
    if(actual < 0) return errno;

    sz -= actual;
    buf += actual;
  }

  return 0;
}

/* Recv a complete stream with error checking. */
int recvall(int sockfd, void *buf, size_t sz)
{
  int actual;

  while(sz > 0)
  {
    errno = 0;
    actual = recv(sockfd, buf, sz, MSG_NOSIGNAL);
    if(actual < 0) return errno;

    sz -= actual;
    buf += actual;
  }

  return 0;
}

/* Send the client a framebuffer update. */
void rfb_serv_update
(struct rfb_threadinfo *tinfop,
 uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  int err;
  int numpxl = w * h;
  int sockfd = tinfop->sockfd;
  uint32_t *buf = tinfop->sinfop->bitmap;
  uint8_t msgtype = 0;
  uint8_t padding = 0;
  uint16_t numrect = PK16(1);
  int32_t enc = htonl(0);
  static uint32_t seed = 0;

  /* Pack coordinates for transmission. */
  x = PK16(x);
  y = PK16(y);
  w = PK16(w);
  h = PK16(h);

  /* Send message header. */
  err = sendall(sockfd, &msgtype, sizeof msgtype);
  if(err) client_die("send() server update message type", err);
  err = sendall(sockfd, &padding, sizeof padding);
  if(err) client_die("send() server update padding", err);
  err = sendall(sockfd, &numrect, sizeof numrect);
  if(err) client_die("send() server update numrect", err);

  /* Send framebuffer header. */
  err = sendall(sockfd, &x, sizeof x);
  if(err) client_die("send() server update x", err);
  err = sendall(sockfd, &y, sizeof y);
  if(err) client_die("send() server update y", err);
  err = sendall(sockfd, &w, sizeof w);
  if(err) client_die("send() server update w", err);
  err = sendall(sockfd, &h, sizeof h);
  if(err) client_die("send() server update h", err);
  err = sendall(sockfd, &enc, sizeof enc);
  if(err) client_die("send() server update encoding", err);

  /* Send the entire bitmap. */
  err = sendall(sockfd, buf, numpxl * sizeof *buf);
  if(err) client_die("send() bitmap", err);
}

/* Negotiate the RFB protocol version. */
void rfb_client_ver(int sockfd, struct rfb_protinfo *infop)
{
  int err;

  /* Send the RFB version string. */
  err = sendall(sockfd, infop->ver, RFBVER_SZ);
  if(err) client_die("send() RFB version", err);

  /* Recv client's RFB version string. */
  err = recvall(sockfd, infop->ver, RFBVER_SZ);
  if(err) client_die("recv() RFB version", err);

  if(MAJ(infop->ver) != 3 || MIN(infop->ver) > 8)
  {
    fprintf(stderr, "RFBv%d.%d not supported. Dying.\n");
    pthread_exit((void*) 1);
  }
  else
    printf("Connected to client using RFBv%d.%d\n",
           MAJ(infop->ver), MIN(infop->ver));
}

/* Negotiate a security type. */
void rfb_client_sec(int sockfd, struct rfb_protinfo *infop)
{
  int err;
  uint32_t v3sec = PK32(infop->sec[0]);
  uint32_t success = PK32(0);
  uint8_t v7sec[infop->seclen + 1];
  uint8_t client_sec = 1;
  uint8_t *sec_err = "Security negotiation failed.";

  /* RFBv3.7+ allows multiple security types. */
  v7sec[0] = (uint8_t) infop->seclen;
  memcpy(v7sec + 1, infop->sec, sizeof v7sec);

  /* Send a security types string. */
  if(MIN(infop->ver) < 7) /* Security string for v3.3 */
    err = sendall(sockfd, &v3sec, sizeof v3sec);
  else /* Security string for v3.7+ */
    err = sendall(sockfd, v7sec, sizeof v7sec);
  if(err) client_die("send() security string", err);

  /* Recv the client's prefered security type for v3.7+. */
  if(MIN(infop->ver) >= 7)
  {
    err = recvall(sockfd, &client_sec, sizeof client_sec);
    if(err) client_die("recv() security type", err);
    infop->sec[0] = UPK32(client_sec);
  }

  /* Handle security negotiation failure. */
  if(!client_sec)
  {
    if(MIN(infop->ver) < 8) /* ...for version < 3.8 */
    {
      success = PK32(1);
      sendall(sockfd, &success, sizeof success);
    } else /* ...for version >= 3.8 */
    {
      success = PK32(strlen(sec_err));
      sendall(sockfd, &success, sizeof success);
      sendall(sockfd, sec_err, success);
    }

    fprintf(stderr, "%s Dying.\n", sec_err);
    pthread_exit((void*) 1);
  }

  /* Otherwise, notify client of success. */
  err = sendall(sockfd, &success, sizeof success);
  if(err) client_die("send() security success", err);

  printf("Using security type %d\n", infop->sec[0]);
}

/* Negotiate server/client init messages. */
void rfb_client_init(int sockfd, struct rfb_protinfo *infop)
{
  int err;
  uint8_t shared, pxlfmt[16];
  uint16_t fbwidth = PK16(infop->fbwidth);
  uint16_t fbheight = PK16(infop->fbheight);
  uint16_t rmax = PK16(infop->pxl.rmax);
  uint16_t gmax = PK16(infop->pxl.gmax);
  uint16_t bmax = PK16(infop->pxl.bmax);
  uint32_t namelen = PK32(infop->namelen);

  /* Recv (and ignore) client shared flag. */
  err = recvall(sockfd, &shared, sizeof shared);
  if(err) client_die("recv() client init", err);

  /* Send framebuffer width / height. */
  err = sendall(sockfd, &fbwidth, sizeof fbwidth);
  if(err) client_die("send() framebuffer width", err);
  err = sendall(sockfd, &fbheight, sizeof fbheight);
  if(err) client_die("send() framebuffer height", err);

  /* Pack the pixel format string (16 bytes total). */
  memset(pxlfmt, 0, sizeof pxlfmt);
  memcpy(pxlfmt, &infop->pxl.bits, 1);
  memcpy(pxlfmt + 1, &infop->pxl.depth, 1);
  memcpy(pxlfmt + 2, &infop->pxl.bigend, 1);
  memcpy(pxlfmt + 3, &infop->pxl.trucol, 1);
  memcpy(pxlfmt + 4, &rmax, 2);
  memcpy(pxlfmt + 6, &gmax, 2);
  memcpy(pxlfmt + 8, &bmax, 2);
  memcpy(pxlfmt + 10, &infop->pxl.rshift, 1);
  memcpy(pxlfmt + 11, &infop->pxl.gshift, 1);
  memcpy(pxlfmt + 12, &infop->pxl.bshift, 1);
  /* + 3 bytes of padding */

  /* Whew! Now send it. */
  err = sendall(sockfd, pxlfmt, sizeof pxlfmt);
  if(err) client_die("recv() pixel format", err);

  /* Send the service name. */
  err = sendall(sockfd, &namelen, sizeof namelen);
  if(err) client_die("send() service name length", err);
  err = sendall(sockfd, infop->name, infop->namelen);
  if(err) client_die("send() service name", err);
}

/* Handle a client set-pixel-format message. */
void rfb_client_set_pxl(int sockfd, struct rfb_protinfo *infop)
{
  int err;
  uint16_t rmax, gmax, bmax;
  uint8_t bits, depth, bigend, trucol;
  uint8_t rshift, bshift, gshift;
  uint8_t padding[3];

  /* Clear 3 bytes of padding. */
  err = recvall(sockfd, padding, sizeof padding);
  if(err) client_die("recv() client pxlfmt pre-padding", err);

  /* Recv bits per pixel / color depth. */
  err = recvall(sockfd, &bits, sizeof bits);
  if(err) client_die("recv() client pixel bits", err);
  err = recvall(sockfd, &depth, sizeof depth);
  if(err) client_die("recv() client pixel depth", err);

  /* Recv big endian / true color flags. */
  err = recvall(sockfd, &bigend, sizeof trucol);
  if(err) client_die("recv() client big endian flag", err);
  err = recvall(sockfd, &trucol, sizeof trucol);
  if(err) client_die("recv() client true color flag", err);

  /* Recv max color values. */
  err = recvall(sockfd, &rmax, sizeof rmax);
  if(err) client_die("recv() client red max", err);
  err = recvall(sockfd, &gmax, sizeof gmax);
  if(err) client_die("recv() client green max", err);
  err = recvall(sockfd, &bmax, sizeof bmax);
  if(err) client_die("recv() client blue max", err);
  infop->pxl.rmax = UPK16(rmax);
  infop->pxl.gmax = UPK16(gmax);
  infop->pxl.bmax = UPK16(bmax);

  /* Recv right bit shift for each color. */
  err = recvall(sockfd, &rshift, sizeof rshift);
  if(err) client_die("recv() client red shift", err);
  err = recvall(sockfd, &gshift, sizeof gshift);
  if(err) client_die("recv() client blue shift", err);
  err = recvall(sockfd, &bshift, sizeof bshift);
  if(err) client_die("recv() client green shift", err);

  /* Clear 3 bytes of padding. */
  err = recvall(sockfd, padding, sizeof padding);
  if(err) client_die("recv() client pxlfmt post-padding", err);

  /* Print a helpful log. */
  fprintf(stderr, 
          "Client changed pixel format:\n"
          "Bits per pixel: %d\n"
          "Endianness of pixels: %s\n"
          "True color: %s\n"
          "Maximum color values: r%d, g%d, b%d\n"
          "Right shift for color bits: r%d, g%d, b%d\n",
          infop->pxl.bits,
          (infop->pxl.bigend) ? "big" : "little",
          (infop->pxl.trucol) ? "yes" : "no",
          infop->pxl.rmax,
          infop->pxl.gmax,
          infop->pxl.bmax,
          infop->pxl.rshift,
          infop->pxl.gshift,
          infop->pxl.bshift);
}

/* Handle a client set-encoding message. */
void rfb_client_set_enc(int sockfd, struct rfb_protinfo *infop)
{
  int err;
  uint8_t padding;
  uint16_t bufsz;
  int32_t *buf;

  /* Clear padding. */
  err = recvall(sockfd, &padding, sizeof padding);
  if(err) client_die("recv() client encoding padding:", err);

  /* Get number of encodings. */
  err = recvall(sockfd, &bufsz, sizeof bufsz);
  if(err) client_die("recv() client encoding size:", err);
  bufsz = UPK16(bufsz);

  /* Get the client's encodings. */
  errno = 0;
  buf = malloc(bufsz * sizeof *buf);
  if(!buf) client_die("malloc() client encoding buffer", err);

  err = recvall(sockfd, buf, bufsz * sizeof *buf);
  if(err) client_die("recv() cllient encoding buffer", err);

  /* ...and completely ignore the request. */
  fprintf(stderr, "%d color encodings recvd. We don't care.\n",
          bufsz);
  free(buf);
}

/* Handle a client framebuffer-update message. */
void rfb_client_update(struct rfb_threadinfo *tinfop)
{
  int err;
  int sockfd = tinfop->sockfd;
  uint8_t incr;
  uint16_t x;
  uint16_t y;
  uint16_t w;
  uint16_t h;

  /* Recv client incremental flag. */
  err = recvall(sockfd, &incr, sizeof incr);
  if(err) client_die("recv() client incremental flag", err);

  /* Recv starting x/y coordinates. */
  err = recvall(sockfd, &x, sizeof x);
  if(err) client_die("recv() client update start x", err);
  x = UPK16(x);
  err = recvall(sockfd, &y, sizeof y);
  if(err) client_die("recv() client update start y", err);
  y = UPK16(y);

  /* Recv update width / height. */
  err = recvall(sockfd, &w, sizeof w);
  if(err) client_die("recv() client update width", err);
  w = UPK16(w);
  err = recvall(sockfd, &h, sizeof h);
  if(err) client_die("recv() client update height", err);
  h = UPK16(h);

  /* Send the requested update. */
  rfb_serv_update(tinfop, x, y, w, h);
}

/* Handle client key-event message. */
void rfb_client_kbd(int sockfd, struct rfb_protinfo *infop)
{
  int err;
  uint8_t down;
  uint8_t padding[2];
  uint32_t key;

  /* Recv client key-down flag. */
  err = recvall(sockfd, &down, sizeof down);
  if(err) client_die("recv() client key-down flag", err);

  /* Clear 2 bytes of padding. */
  err = recvall(sockfd, padding, sizeof padding);
  if(err) client_die("recv() client key padding", err);

  /* Recv X keycode. */
  err = recvall(sockfd, &key, sizeof key);
  if(err) client_die("recv() client keycode", err);
  key = UPK32(key);

  /* Log to stdout. */
  fprintf(stderr, "Client keyboard event: key %#x %s\n",
         key, (down) ? "down" : "up");
}

/* Handle client pointer-event message. */
void rfb_client_ptr(struct rfb_threadinfo *tinfop)
{
  int err;
  int sockfd = tinfop->sockfd;
  struct rfb_servinfo *sinfop = tinfop->sinfop;
  struct rfb_protinfo *infop = &sinfop->pinfo;
  uint16_t x;
  uint16_t y;
  uint8_t buttmask;
  struct rfb_event *ev_temp;

  /* Recv button mask (1 bit each for buttons 1-8). */
  err = recvall(sockfd, &buttmask, sizeof buttmask);
  if(err) client_die("recv() client mouse button mask", err);

  /* Recv cursor x/y-coordinates. */
  err = recvall(sockfd, &x, sizeof x);
  if(err) client_die("recv() client cursor x", err);
  x = UPK16(x);
  err = recvall(sockfd, &y, sizeof y);
  if(err) client_die("recv() client cursor y", err);
  y = UPK16(y);

  /* Queue the pointer event, if it's relevant. */
  if(x < infop->fbwidth && y < infop->fbheight && buttmask)
  {
    ev_temp = malloc(sizeof *ev_temp);
    ev_temp->color = tinfop->client.color;
    ev_temp->buttmask = buttmask;
    ev_temp->x = x;
    ev_temp->y = y;
    pthread_mutex_lock(&sinfop->event_mutex);
      ev_temp->next = sinfop->event_list;
      sinfop->event_list = ev_temp;
    pthread_mutex_unlock(&sinfop->event_mutex);
  }

  /* Log. */
  fprintf(stderr, "Client pointer event: %#x at (%d, %d)\n",
         buttmask, x, y);
}

/* Handle client cut-text message. */
void rfb_client_cut_txt(int sockfd, struct rfb_protinfo *infop)
{
  int err;
  uint8_t padding[3];
  uint32_t bufsz;
  uint8_t *buf;

  /* Clear 3 bytes of padding. */
  err = recvall(sockfd, &padding, sizeof padding);
  if(err) client_die("recv() client cut text padding", err);

  /* Recv the size of the cut buffer. */
  err = recvall(sockfd, &bufsz, sizeof bufsz);
  if(err) client_die("recv() client cut buffer size", err);
  bufsz = UPK32(bufsz);

  /* Recv the clent cut buffer. */
  errno = 0;
  buf = malloc((bufsz * sizeof *buf) + 1);
  if(!buf) client_die("malloc() cut text buffer", errno);

  err = recvall(sockfd, buf, bufsz * sizeof *buf);
  if(err) client_die("recv() client cut buffer", err);
  buf[bufsz - 1] = '\0';

  /* ...and disgard it. */
  fprintf(stderr, "Recvd a %d byte cut text buffer: %s",
          bufsz, buf);
  free(buf);
}

/* Negotiate a client connection. */
void rfb_handshake(int sockfd, struct rfb_protinfo *infop)
{
  int err;

  /* Version negotiation */
  rfb_client_ver(sockfd, infop);

  /* Security negotiation */
  rfb_client_sec(sockfd, infop);

  /* Server/client initialization */
  rfb_client_init(sockfd, infop);
}

/* Client handler for pthread_create() */
void *rfb_handle_client(void *argp)
{
  int err;
  struct rfb_threadinfo *tinfop = argp;
  int sockfd = tinfop->sockfd;
  struct rfb_protinfo *pi = &tinfop->sinfop->infop;
  uint8_t buf;

  rfb_handshake(sockfd, pi);
  
  /* Handle client -> server messages in a loop. */
  while(1)
  {
    err = recv(sockfd, &buf, 1, 0);
    if(err < 0) perror("recv()");

    switch((uint16_t)buf)
    {
      case 0: rfb_client_set_pxl(sockfd, pi); break;
      case 2: rfb_client_set_enc(sockfd, pi); break;
      case 3: rfb_client_update(tinfop); break;
      case 4: rfb_client_kbd(sockfd, pi); break;
      case 5: rfb_client_ptr(tinfop); break;
      case 6: rfb_client_cut_txt(sockfd, pi); break;
      default:
        fprintf(stderr, "Unknown client message: %d\n", buf);
        break;
    }
  }
  
  return NULL;
}

/* Return and dequeue (but not free!) last event in list. */
rfb_event_t *rfb_event_next(rfb_event_t *ev_list)
{
  struct rfb_event *ev_prev;

  /* Find the end of the list. */
  while(ev_list->next)
  {
    ev_prev = ev_list;
    ev_list = ev_list->next;
  }

  /* Dequeue and return. */
  ev_prev->next = NULL;
  return ev_list;
}

/* Free all memory associated with the server thread. */
void rfb_thread_cleanup(void *argp)
{
  struct rfb_threadinfo **tlistp = argp;
  struct rfb_threadinfo *tlist = *tlistp;
  struct rfb_threadinfo *temp;
  struct rfb_event *cur_ev = tlist->sinfop->event_list;
  struct rfb_event *prv_ev;

  /* Deallocate non-thread stuff. */
  free(tlist->sinfop->bitmap);
  pthread_mutex_destroy(&tlist->sinfop->event_mutex);
  pthread_mutex_destroy(&tlist->sinfop->client_mutex);
  while(prv_ev = cur_ev)
  {
    cur_ev = cur_ev->next;
    free(prv_ev);
  }

  /* Deallocate thread stuff till we can deallocate no more. */
  while(temp = tlist)
  {
    tlist = tlist->next;
    close(temp->sockfd);
    free(temp);
  }
}

/* Accept client connections and pass in requisite data. */
void *rfb_accept(void *argp)
{
  int err;
  int client_sockfd;
  int seed = time(0);
  static uint32_t colors[6] = COLORS;
  struct rfb_servinfo *sinfop = argp;
  struct rfb_threadinfo *thread_list = NULL;
  struct rfb_threadinfo *ti_temp;

  pthread_cleanup_push(rfb_thread_cleanup, &thread_list);

  /* Accept incoming connections in a loop. */
  while(1)
  {
    client_sockfd = accept(sinfop->sockfd, NULL, NULL);
    if(client_sockfd < 0) {perror("accept()"); continue;}

    /* Create new thread info. Hold onto your butts... */
    ti_temp = malloc(sizeof *ti_temp);
    memset(ti_temp, 0, sizeof *ti_temp);
    ti_temp->next = thread_list;
    thread_list = ti_temp;
    pthread_mutex_lock(&sinfop->client_mutex);
      ti_temp->client.next = sinfop->client_list;
      sinfop->client_list = &ti_temp->client;
    pthread_mutex_unlock(&sinfop->client_mutex);
    ti_temp->sinfop = sinfop;
    ti_temp->sockfd = client_sockfd;
    ti_temp->client.color = colors[rand_r(&seed) % 6];

    /* Fork a new client pthread. */
    err = pthread_create(&ti_temp->thr, NULL,
                         rfb_handle_client, ti_temp);
    if(err) /* FIXME: Remove thread info on error. */
    {
      perror("pthread_create()");
      close(client_sockfd);
      continue;
    }
  }

  pthread_cleanup_pop(1);

  return NULL;
}

/* Prepare and return server info. */
rfb_servinfo_t *rfb_run(int w, int h, char *name, char *serv)
{
  int err;
  int sockfd;
  int mapsz;
  struct rfb_protinfo rfbproto_dft = RFBPROTO_DFT;
  struct rfb_servinfo *sinfop = malloc(sizeof *sinfop);

  /* Fill in supplied protocol info, if it exists. */
  sinfop->infop = rfbproto_dft;
  if(w) sinfop->infop.fbwidth = w;
  if(h) sinfop->infop.fbheight = h;
  if(name)
  {
    sinfop->infop.name = name;
    sinfop->infop.namelen = strlen(name);
  }

  /* Create an empty bitmap to use as a shared buffer. */
  mapsz = sinfop->infop.fbwidth * sinfop->infop.fbheight;
  sinfop->bitmap = malloc(mapsz * sizeof *sinfop->bitmap);
  memset(sinfop->bitmap, 0, mapsz * sizeof *sinfop->bitmap);

  /* Prepare server info. */
  sinfop->event_list = NULL;
  sinfop->client_list = NULL;
  err = pthread_mutex_init(&sinfop->event_mutex, NULL);
  if(err) {perror("pthread_mutex_init()"); return NULL;}
  err = pthread_mutex_init(&sinfop->client_mutex, NULL);
  if(err) {perror("pthread_mutex_init()"); return NULL;}

  /* Prepare the service socket. */
  sockfd = rfb_sockinit(serv);
  errno = 0;
  err = listen(sockfd, BKLG_DFT);
  if(err) std_die("listen()", err);
  sinfop->sockfd = sockfd;

  /* Fork a thread to accept incoming connections. */
  err = pthread_create(&sinfop->thr, NULL, rfb_accept, sinfop);
  if(err) std_die("pthread_create()", err);
  pthread_detach(sinfop->thr);

  return sinfop;
}

/* Cancel the server thread. */
void rfb_kill(rfb_servinfo_t *sinfop)
{pthread_cancel(sinfop->thr);}

