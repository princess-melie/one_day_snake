#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <math.h>
#include <vector>
//Я тут много указал, предположу что у тебя может быть msvc
//вместо компилятора, и он требует всё указать
//mingw компилирует это с указанием двух заголовочных файлов





/*
 Есть класс t_winapi_window, это окно windows и обработка событий. Он просто с сайта
 микрософт скопирован

 В нём есть класс t_software_render, там функции рендера (и ещё кусочек winapi),
 эта же штука пересылает сообщения уже в игру.

 t_mode - базовый интерфейс игры, от него наследуются t_mode_snake
(код непосредственно змейки), и эти t_mode хранятся в t_software_render, который ими рулит
Событий взаимодействия игрой с окном минимум - это события клаво-мышки и событие перерисовки.


Там часть слов транслитом, но я не знаю как на английском такие сложные слова как "еда" и "корпус".
Вообще так не делают, и стоило бы посмотреть в переводчике, но любая современная ide должна позволять
переименовать за 10 секунд это.






Чуть-чуть шизы от себя:
  Тут виртуальное методы и наследование. Это ебаные костыли (хуже только исключения)
  и я очень надеюсь что в крупных програмах их используют по минимому.
На самом деле не правда. Если исключения - концепция сомнительная и есть часть проектов уровня хромиума, где полностью
отказались от использования исключений, более того их меняли в совсем молодых стандартах 14 и 17 года.
То вот виртуальные методы полезны и с ними код может быть (это важно) намного быстрее в разработке
и понятнее (а значит при его разработки будет меньше человеческих ошибок и процесс будет быстрее).
  Тем не менее запомни сразу что шизу "всё есть объект" нужно гнать, это религиозное уже.
ООП, и книжка по паттернам проектирования - хороший инструмент и там есть полезные штуки, но
нужно думать как сделать работающую программу, а не как загнать нужно задачу в паттерны и решать
её только с помощью паттернов.
  И то же самое с наследованием - если у тебя цепочка из десяти наследований, есть множественные
  наследования - то код конечного класса будет разнесён по десяти разными местам, что очень сложно
  разрабатывать и поддерживать. В общем удачи, пиши хороший код)


 //*/





// Это анонимный неймспейс, он вообще для другого служит
// Но я его написал чтобы в ide можно было этот блок свернуть
namespace {

   using byte = uint8_t;///число на 1 байт со значениями 0..255

   ///Это просто синтаксический сахар, чтобы было удобнее пользоваться
   ///4 байта, в windows хранятся в режиме bgra вроде бы
   union pixel {
      pixel(byte r, byte g, byte b, byte a): b(b), g(g), r(r), a(a) {}
      pixel(byte r, byte g, byte b): b(b), g(g), r(r), a(255) {}
      struct {
         byte b, g, r, a;
      };
      unsigned col;
   };

   ///Преобразовать флоат 0..1 в целое число 0..255, так более по человечески
   byte f32_t_u8(float x){
      int k=x*255;
      if (k>255)
         return 255;
      else if(k<0)
         return 0;
      else
         return k;
   }

   ///Тоже синтаксический сахар
   pixel rgb(float r,float g,float b){
      return pixel{f32_t_u8(r),f32_t_u8(g),f32_t_u8(b),255};
   }
   pixel rgb(float r,float g,float b,float a){
      return pixel{f32_t_u8(r),f32_t_u8(g),f32_t_u8(b),f32_t_u8(a)};
   }


}

struct t_software_render;

struct t_mode{
   int next_mode=-1;//Метка, что нужно переключить на другой режим

   t_software_render& render;//Ссылка на вышестоящий класс, без него режимы не существуют
   t_mode(t_software_render& render):render{render}{};
   //По некоторым причинам конструкторы не бывают виртуальными, и придётся в каждом
   //режима такое дублировать
   //Можно вместо ссылки использовать указатель и сделать с нормальным интерфейсом функцию init
   //Но тогда придётся всюду -> писать вместо .
   //
   //Можно конечно подменить значение ссылки через костыли, но это небезопасная,
   //компиляторо и платформозависимая дичь с UB, которая может перестать работать,
   //и это не соответствует назначению ссылки



   // Вот и все функции взаимодействия с окном.
   //Колёсика мышки нет, как и нажатия колёсиком, как и кучи всего остального
   //Стоит добавить события получения и потери фокуса окна
   virtual void init() {}; //Инициализация
   virtual void kd(int key) {}; //Нажатие клавиши
   virtual void ku(int key) {}; //Отжатие клавиши
   virtual void mm(float rx,float ry) {}; //Движения мышью
   virtual void paint(float dt) {}; //Перерисовка
   virtual ~t_mode(){};
};









/// Тут ниже идут функции рендера линий


///Эта структура с логикой меню и рендером
struct t_software_render{

   /*Я не люблю gdi, и gdi+, нарисую вручную
   Это структура создаёт окно, и реализует буфер и простейшие функции рисования
     */

   int wi=0, he=0;  ///размеры окна в пикселях
   float bx=1.6, by=1.0; //размеры экрана в дробных координатах от -1.6 до +1.6 по х
   float xa,xb,ya,yb;// Матрица преобразования экранных коодинат в дробные.
   // Так сделано в openlg, и я нахожу это удобным
   // rx=x*xa+xb
   // x=(rx-xb)/xa
   inline float x_to_rx(int x){return x*xa+xb;}
   inline float y_to_ry(int y){return y*ya+yb;}
   inline int rx_to_x(float rx){return (rx-xb)/xa;}
   inline int ry_to_y(float ry){return (ry-yb)/ya;}

   int active_mode=0;//Активный режим игры
   /*
   0 - выбор режим
   1 - змейка
   2 - сбивать хуету летающую
   //*/
   const int MODES=3; //Количество режимов

   std::vector<t_mode*> modes; //список режимов, я решил и змейку сделать, и не змейку.
   std::vector<bool> keys; // Нажатые в текущий момент клавиши



   ~t_software_render(){
      for (int i=0;i<MODES;i++){
         if (modes[i]){
            delete modes[i];
            modes[i]=nullptr;
         }
      }
   }

   //Реализаця ниже, пришлось одну функцию вынести, что необходимо
   //при такой иерархии классов.
   t_software_render();


   void init(){
      for (int i=0;i<MODES;i++)
         modes[i]->init();
   }
   //Нажатие клавиши
   void kd(byte key){
      keys[key]=true;
      modes[active_mode]->kd(key);

   }
   //Отпуск клавиши
   void ku(byte key){
      keys[key]=false;
      modes[active_mode]->ku(key);
   }
   //Движение мышки
   void mm(int x, int y){
      modes[active_mode]->mm(x_to_rx(x),y_to_ry(y));

   }
   //перерисовка (и время от предыдущего кадра
   void paint(float dt){
      if (modes[active_mode]->next_mode>=0 and modes[active_mode]->next_mode<MODES){
         int nx=modes[active_mode]->next_mode;
         modes[active_mode]->next_mode=-1;
         active_mode=nx;
      }
      modes[active_mode]->paint(dt);

      //Рамка, в которой всё рендерится и к которой масштабируется
      line_p(-bx,-by,-bx,+by,pixel(255,255,255),4);
      line_p(-bx,-by,+bx,-by,pixel(255,255,255),4);
      line_p(+bx,+by,-bx,+by,pixel(255,255,255),4);
      line_p(+bx,+by,+bx,-by,pixel(255,255,255),4);
   }



   pixel& pix(int x,int y)
   {
      static pixel temp{255,255,255};
      if (x>=0 and x<wi and y>=0 and y<he)
         return buf[y*wi+x];
      else
         return temp;
   }
   //Линия по целочисленным координатам
   void line(float x1,float y1,float x2,float y2,pixel col=pixel(255,255,255)){
      line_ip(rx_to_x(x1),ry_to_y(y1),rx_to_x(x2),ry_to_y(y2),col,0);
   }
   void line_p(float x1,float y1,float x2,float y2,pixel col,int p){
      line_ip(rx_to_x(x1),ry_to_y(y1),rx_to_x(x2),ry_to_y(y2),col,p);
   }

   void rect(float x1,float x2,float y1,float y2,pixel col=pixel(255,255,255)){
      rect_i(rx_to_x(x1),rx_to_x(x2),ry_to_y(y1),ry_to_y(y2),col);
   }

   void fill(pixel col){
      std::fill(buf,buf+wi*he,col);
   }

   void rect_i(int x1,int x2,int y1,int y2,pixel col){
      if (x1>x2)
         std::swap(x1,x2);
      if (y1>y2)
         std::swap(y1,y2);

      for (int iy=y1;iy<y2;iy++)
         for (int ix=x1;ix<x2;ix++)
            pix(ix,iy)=col;
   }

   void line_ip(int x1,int y1,int x2,int y2,pixel col,int ip){/*
       for( int x = x1; x<=x2; x++){
           int y=y1+(y2-y1)*(x-x1)/(x2-x1);
           pix(x,y)=rgb(1,1,1);
       }*/

      int dx=x2-x1;
      int dy=y2-y1;
      float dlina=sqrt(dx*dx+dy*dy);
      if (ip<=0)
         for (float e=0;e<=1;e+=1/dlina){
            int x=x1+dx*e;
            int y=y1+dy*e;
            pix(x,y)=col;
         }
      else
      {
         int t=0;
         for (float e=0;e<=1;e+=1/dlina){
            int x=x1+dx*e;
            int y=y1+dy*e;
            if (0==(t/ip)%2)
               pix(x,y)=col;
            t++;
         }

      }

   }
   ///

   pixel* buf=0;
   HDC hdcmem;
   HBITMAP hbmp;
   //Хотелось сделать реализацию этого класса без winapi, но тут выяснилось что можно прям функцией
   // winapi выделять память, но сюда придётся передавать идентификаторы winapi
   //Грязно, но зато память не дублируется.
   //С точки зрения кода было бы лучше сделать дублирующиеся 12 мб памяти,
   //Но чистый класс без winapi дескрипторов
   //
   //Уже придумал как исправить, напишешь если нужно переделать "лучше"
   //Функционально ничего не поменяется кроме чистоты кода
   void resize(HWND hwnd,HDC hdc){
      //Функция вызывается каждый раз. Ага, можно сделать обработку wm_size (и это
      // будет лучше), но я уже сделал так - не в этом суть
      RECT r;
      GetClientRect(hwnd, &r);//Получаю размер окна (внутренней клиентской части)
      int ux,uy;
      ux = r.right - r.left, uy = r.bottom - r.top;

      if (ux==wi and uy==he) //Если размеры совпадает, если ничего не делать
         return;
      if (ux<=2 or uy<=2) //Если в окне нет пикселей - просто удаляю буфер если он есть
      {
         wi=0;
         he=0;
         free();
         return;
      }
      printf("resize %dx%d -> %dx%d\n",wi,he,ux,uy);
      wi=ux;
      he=uy;
      {  // определяю преобразование координат пиксельных в дробные
         float ex,ey; //Реальное соотношение сторон
         if ((float)wi/he>bx/by){  //Если окно слишком сильно растянуто в бока
            ey=by;
            ex=by*wi/(he-1);
         }else{ //Если слишком растянуто вверх-вниз
            ex=bx;
            ey=bx*he/(wi-1);
         }
         /*
         rx == -ex    x=0
         rx == +ex    x=wi
         ry == -ey    y=he
         ry == +ey    y=0

         rx=x*xa+xb
         x=(rx-xb)/xa

         Это просто чтобы преобразование координат написать
         //*/
         xb=-ex;
         xa=(ex-xb)/(wi-1);

         yb=ey;
         ya=(-ey-yb)/(he-1);

         //printf("create max  %.5f %.5f %.5f %.5f\n",xa,xb,ya,yb);

      }
      // x=0  rx= -bx


      if (buf){ //Если буфер был выделен (под предыдущий размер)
         free(); //то очищаю его
      }
      //И создаю новый (это я тоже откудато уровня stack_overflow скопировал
      BITMAPINFO bitmap;
      bitmap.bmiHeader.biSize = sizeof(bitmap.bmiHeader);
      bitmap.bmiHeader.biWidth = wi;
      bitmap.bmiHeader.biHeight = -he;
      bitmap.bmiHeader.biPlanes = 1;
      bitmap.bmiHeader.biBitCount = 32;
      bitmap.bmiHeader.biCompression = BI_RGB;
      bitmap.bmiHeader.biSizeImage = wi * 4 * he;
      bitmap.bmiHeader.biClrUsed = 0;
      bitmap.bmiHeader.biClrImportant = 0;
      hdcmem = CreateCompatibleDC(hdc);
      //Вот эту фигню из (нихрена не) первого попавшегося примера из сети стащил
      hbmp = CreateDIBSection(NULL, &bitmap, DIB_RGB_COLORS, (void **)(&buf), NULL, 0);
      SelectObject(hdcmem, hbmp);
      //printf("create dib %dx%d %x %x\n",wi,he,hdcmem,hbmp);
   }

   void free(){
      if (buf){
         DeleteObject(hbmp);
         DeleteObject(hdcmem);
         buf=0;
      }
   }

};


//////////////////////////////////////////////////////////
///////////       НАЧИНАЕТСЯ КОД ИГРЫ     ////////////////
//////////////////////////////////////////////////////////

/// Код меню
struct t_mode_menu:t_mode{
   float mx,my;
   t_mode_menu(t_software_render& render):t_mode(render){}
   void init(){}
   void kd(int key){
      printf("%f %f",mx,my);
      if (key==1 or key==2){
         if (my>-0.3 and my<0.3)
         {
            if (mx>-0.5 and mx<-0.1)
               next_mode=1;
            if (mx>0.1 and mx<0.5)
               next_mode=2;
         }
      }
   }
   void ku(int key){}
   void mm(float rx,float ry){
      my=ry;
      mx=rx;
   }
   void paint(float dt){
      render.fill(pixel(0,20,0));//Тёмно зелёный фон, заливаю им всё

      //Кнопки смены режима
      render.rect(-0.5,-0.1,-0.3,+0.3,pixel(0,255,0));
      render.rect(0.5,0.1,-0.3,+0.3,pixel(255,0,0));
   };
};


/// Код змейки
/// Управление WASD + R(сброс)
struct t_mode_snake:t_mode{


   std::vector<int> buffer;//Игровое поле
   int xx=32,yy=20; //Его размеры
   int len; //Длина змеи
   int zx,zy; //текущее положение змеи
   int nx,ny; //Направление движения
   static constexpr int KORM = -13; //Специальное значение для корма
   static constexpr int STENA = -52243; //Специальное значение для стен
   float t=0,lt=0;//Время последнего хода, и просто время



   void spawn_korm(){ //Создаёт кусочек корма в свободной ячейке,
      //если таких нет - оно уйдёт в бесконечный цикл (без защиты в виде t)
      //А если их мало - будет очень много считать
      //(всё ещё мало, но в 20 раз больше, чем могло бы (если заполнено 19/20))
      //И в случае если карта заполнена больше чем на 2/3
      //(три броска случайных чисел тоже не сложные)
      //То стоит создать список координат с пустыми клетками, и выбирать
      //случайную клетку из них, а не из всех и проверять пустая ли она
      int ux,uy;

      int t=0;//Считаю тут итерации
      do{
         t++;
         if (t>10000) return; //Выключаю, если не может найти клетку
         //Шанс минимальный, но не нулевой.
         ux=rand()%xx;
         uy=rand()%yy;
         //Ищу пустую клетку
         //С помощью рандома со времён мамонтов
      }while(buffer[uy*xx+ux]!=0);

      buffer[uy*xx+ux]=KORM;
   }

   void die(){ //Полный ресет всего
      len=4; //Стартовая длина змейки - 4
      std::fill(buffer.begin(),buffer.end(),0); //Всё поле заполняю пустыми клетками
      zx=xx/2;
      zy=yy/2;//змейка в центре поля
      nx=1;
      ny=0; //Двигается направо
      buffer[zx+xx*zy]=len;
      for (int x=0;x<xx;x++)
         buffer[x+xx*3]=STENA; //Рисую стену полоской
      spawn_korm(); //И ставлю один корм
   }
   t_mode_snake(t_software_render& render):t_mode(render){ //В конструкторах-деструкторая
      //я только освобождаю и занимаю ресурсы
      //То есть прям минимум без которого работать не будет
      //инициализацию провожу отдельно и явно в функции init
      //Почти точно так делают не все и возможно это плохая концепция
      buffer.resize(yy*xx,0); //Это контейнер stl, он сам очистится в деструкторе
   }
   void init(){  //А вот это уже явная инициализация
      die(); //Никаких результатов не сохраняется, потому функция общая со смертью
   }


   bool lock=false;//Блокировка, чтобы резким
   //нажачием и разворотом на 180 не убивать себя, если нажать например w+a при движении влево
   //Между двумя тиками
   void tick(){ //Один "ход" звейки
      for (int y=0;y<yy;y++)
         for (int x=0;x<xx;x++)
         {
            if (buffer[x+y*xx]>0){
               buffer[x+y*xx]--; //Во всех клетках змейки время их существования снижаю
               continue;
            }

         }
      int hx=(zx+nx+xx)%xx,hy=(zy+ny+yy)%yy; //Куда змея хочет попасть
      if (buffer[hx+xx*hy]>0 or buffer[hx+xx*hy]==STENA){
         die(); //Умирает если стена или её хвост
         return;
      }else if (buffer[hx+xx*hy]==KORM){
         len+=1; //Растёт, если тут корм
         spawn_korm();
      }
      zx=hx;
      zy=hy;   //Обновляю координаты
      lock=false; //И разрешаю следующую клавишу управления нажимать
      buffer[zx+xx*zy]=len; //Заполняю клетку там где голова

   }
   float mx,my;//Тут координаты мышки
   void mm(float rx,float ry){
      my=ry;
      mx=rx;
   }
   void kd(int key){
      if (key=='R')
      {
         die();  //Сброс игрового поля
         return;
      }
      if (key==1){  //Выход в меню через красный квадрат в углу
         if (my>render.by-0.1 and mx<-render.bx+1)
            next_mode=0;
      }

      if (lock)
         return;
      if (nx==0){ //Если двигаюсь вертикально
         if (key=='A'){
            nx=-1;
            ny=0;
            lock=true;
         }else if (key=='D'){
            nx=+1;
            ny=0;
            lock=true;
         }
      }else if (ny==0){//Если двигаюсь горизонтально
         if (key=='S'){
            nx=0;
            ny=-1;
            lock=true;
         }else if (key=='W'){
            nx=0;
            ny=+1;
            lock=true;
         }
      }
   }

   void paint(float dt){
      render.fill(pixel(0,0,20)); //Тёмно синий фон
      float dx=std::min(render.bx/xx,render.by/yy)*1.8; //Размер рисуемой клетки

      t+=dt;
      if (t-lt>0.16666){
         tick(); //Если прошла 1/6 секунды - змейка делает "ход"
         //Тут можно ускорить или замедлить игру
         lt=t;
      }




      for (int y=0;y<yy;y++)
         for (int x=0;x<xx;x++)
         { //Просто разными цветами все клетки рисую через мой костыльнй api рисования
            pixel c(40,40,40);
            if (buffer[x+y*xx]==STENA)
               c=pixel(255,0,0);
            else if  (buffer[x+y*xx]==KORM)
               c=pixel(127,127,255);

            else if  (buffer[x+y*xx]==len)
               c=pixel(255,255,255);
            else if  (buffer[x+y*xx]>0)
               c=pixel(127,127,127);
            render.rect((x+0.1-xx/2)*dx,(x+0.9-xx/2)*dx,(y+0.1-yy/2)*dx,(y+0.9-yy/2)*dx,c);

         }

      //Шкала длины змейки
      float score=-3.65905+0.669735*log(len+50); //Я просто вбил прогрессию желаемую,
      //формулу, и два числа регрессией подогнал под нужный диапазон
      render.rect(-render.bx,-render.bx*0.95,-render.by,render.by*score,pixel(255,255,0));

      //Шкала времени змейки
      float time=-3.56988+0.656917*log(t+50);
      render.rect(render.bx,render.bx*0.95,-render.by,render.by*time,pixel(255,0,255));
      //Кнопка выхода
      render.rect(-render.bx,-render.bx+0.1,render.by,render.by-0.1,pixel(255,0,0));
   }

};

//Вершина
union vec2{
   struct{
      float x,y;
   };
   float raw[2];
};


//Треугольник
struct t_form{
   t_form(){}
   t_form(float x1,float y1,float x2,float y2,float x3,float y3,float q=1):
      x1{x1},y1{y1},
      x2{x2},y2{y2},
      x3{x3},y3{y3},
      q{q}
   {
      float mx=(x1+x2+x3)/3,my=(y1+y2+y3)/3;
      for (int i=0;i<3;i++){
         pp[i].x-=mx;
         pp[i].y-=my;
      }
   }
   //Это чтобы можно было любым способом обращаться к вершинами как удобно
   union{
      struct{
         float x1,y1,x2,y2,x3,y3;
      };
      struct {
         vec2 p1,p2,p3;
      };
      struct{
         vec2 pp[3];
      };
      struct{
         float raw[6];
      };
   };
   float q,v,m;//Плотность, объём, масса и момент инерции

};

//Это позиция, в 3d было бы матрицей, или кватернионом+вектором
struct t_mat{
   float x,y,a;//Перенос и угол (против часовой)
   t_form operator()(t_form w){
      t_form ret;
      //!!Место где остановился
      return ret;
   }
   vec2 operator()(vec2 w){
      float s,c;
      sincosf(a,&s,&c); //Эту штука не constexpr
      //И меняет состояние глобальных переменных (какой-то флаг там что ли)
      // constexpr float r=cos(s); - то есть это не компилируется
      //И потому я не уверен что оно проведёт оптимизацию если поставить два cos(a) ниже,
      //так как оно никак не может узнать что мне не важно вышеупомянутое изменение
      //
      // https://godbolt.org/z/1PqMhWeon
      // Хотя годботл для моей версии компилятора пишет что уже всё ок и вызов один
      // При этом там же clanq делает четыре вызова, как и msvc (кстати, в нём вовсе нет sincos)
      // То есть эти две строчки кода не лишние, если кто-то с clanq будет компилировать
      return vec2{x+c*w.x-s*w.y,y+c*w.y+s*w.x};
      // return vec2{x+cos(a)*w.x-sin(a)*w.y,y+cos(a)*w.y+sin(a)*w.x};
   }
};

struct t_korpus{
   t_form form,real_form;
   t_mat mat;
   pixel col;//Цвет
   void draw(t_software_render& render){
      render.line(real_form.x1,real_form.y1,real_form.x2,real_form.y2,col);
      render.line(real_form.x2,real_form.y2,real_form.x3,real_form.y3,col);
      render.line(real_form.x3,real_form.y3,real_form.x1,real_form.y1,col);
   }
   void re_calc(){
      real_form = mat(form);
   }
};

/// Код аркады
struct t_mode_2:t_mode{

   t_mode_2(t_software_render& render):t_mode(render){}
   float nx;//Направление ходьбы
   float xx=0; //Игрок
   void paint(float dt){
      render.fill(pixel(20,20,20)); //Тёмно серый фон

      nx=0;
      if (render.keys['A'])
         nx+=-1;
      if (render.keys['D'])
         nx+=+1;
      xx+=nx*dt;


      render.line(xx,0,xx,-0.1);





      //Кнопка выхода
      render.rect(-render.bx,-render.bx+0.1,render.by,render.by-0.1,pixel(255,0,0));

   }

   void kd(int key){
      if (key==1){  //Выход в меню через красный квадрат в углу
         if (my>render.by-0.1 and mx<-render.bx+1)
            next_mode=0;
      }
   }

   float mx,my;//Тут координаты мышки
   void mm(float rx,float ry){
      my=ry;
      mx=rx;
   }

   void reset(){

   }

   void init(){
      reset();

   }
};




//////////////////////////////////////////////////////////
///////////      ЗАКАНЧИВАЕТСЯ КОД ИГРЫ   ////////////////
//////////////////////////////////////////////////////////


t_software_render::t_software_render(){// В конструктере только освобождаются и занимаются ресурсы.
   // Инициализация уже по отдельной команде в явно указанный момент кода
   keys.resize(256,false);
   modes.resize(MODES);
   modes[0]=new t_mode_menu(*this);
   modes[1]=new t_mode_snake(*this);
   modes[2]=new t_mode_2(*this);
}



///Эта структура чисто обёртка вокруг winapi
struct t_winapi_window{
private:
   HDC hdc;//Дескриптор того, на чём рисуется
   HWND hwnd; //Дескриптор окна
   HINSTANCE histance; //Ещё какой-то дескриптор
   char clname[32]=" :: zmeika_for_pr ::"; //Ещё какой-то внутренний идентификатор winapi для окна

public: ///Единственная доступная снаружи функция - это запуск
   //сюда ещё остановку можно добавить, больше ничего не нужно

   void run(){

      // ----------- Код ниже я просто скопировал из примера создания окна -----
      histance = GetModuleHandleW(NULL);
      WNDCLASSEX wclass = { 0 };
      wclass.cbSize = sizeof(WNDCLASSEX);
      wclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
      wclass.lpfnWndProc = t_winapi_window::WndProc; //Это моя оконная функция
      wclass.hInstance = histance;
      wclass.hCursor = LoadCursor(NULL, IDC_CROSS); //Курсор зачем-то поменял
      wclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
      wclass.lpszClassName = clname;
      RegisterClassEx(&wclass);

      hwnd = CreateWindowEx(
                WS_EX_APPWINDOW | WS_EX_WINDOWEDGE, // Extended Style For The Window
                clname, // Class Name
                " :: zmeika_for_pr ::", // Window Title
                WS_OVERLAPPEDWINDOW | // Defined Window Style
                WS_CLIPSIBLINGS | // Required Window Style
                WS_CLIPCHILDREN | WS_SYSMENU, // Required Window Style
                200,
                100, // Window Position
                800, // Calculate Window Width
                600, // Calculate Window Height
                NULL, // No Parent Window
                NULL, // No Menu
                histance, // Instance
                this); //Оконная функция статичная - через это поле можно передать ей указатель
      // --------------------------------------------------

      _init();
      hdc = GetDC(hwnd);
      ShowWindow(hwnd, SW_NORMAL);
      MSG msg = {0};
      while (GetMessage(&msg, 0, 0, 0)) {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
   }

private:
   t_software_render game;

   //"Чистые" функции, тут просто чтобы в логе
   //что-то было, просто пересылаю в game события

   //инициализация
   void _init(){
      printf("init\n");
      game.init();
   }

   //Нажатие клавиши
   void _kd(int key){
      printf("kd [%d]\n",key);
      game.kd(key);

   }
   //Отпуск клавиши
   void _ku(int key){
      printf("ku [%d]\n",key);
      game.ku(key);

   }
   //Движение мышки
   void _mm(int x, int y){
      game.mm(x,y);

   }
   //перерисовка (и время от предыдущего кадра)
   void repaint(float dt){

      game.resize(hwnd,hdc);
      //printf("repaint [dt = %.1f ms]\n",dt*1000);
      game.paint(dt);

      BitBlt(hdc, 0, 0, game.wi, game.he, game.hdcmem, 0, 0, SRCCOPY);
   }

   //Ниже идут внутренние детали реализации, которые для кода игры смысла не имеют
   long last_clock=clock(); //Время последнего обновления
   float fps=60;
   void paint(){
      long now = clock(); //Там два события, wm_paint и wm_erasebkgnd
      if (now-last_clock>CLOCKS_PER_SEC*0.015) //Если прошло больше 15 мс
      {
         float dt=((float)(now-last_clock))/CLOCKS_PER_SEC;
         //Это один из самых "наколенных"
         //способов получить время между кадрами быстро

         fps=(fps*(1-dt*3)+3);//Тут вообще экспонента должна быть,
         //это прям грубый способ решить численно диффуру без экспоненты

         if (dt>0.1) dt=0.1;// Чтобы не было странных вещей в первый кадр
         // И на случай если залагает что-то в системе и приложение фризанёт
         char ss[32];
         sprintf(ss,"fps = %.1f mode = %d",fps,game.active_mode);
         SetWindowTextA(hwnd,ss);
         last_clock = now;
         //размер окна изменился
         //Можно конечно обрабатывать wm_size
         //Но это желающие могут сделать самостоятельно
         repaint(dt); //собственно перерисовка
      }
   }



   ///Оконная функция
   static LRESULT WINAPI WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
   {
      t_winapi_window* self = 0; //Вот сюда упадёт указатель на экземпляр t_software_render
      if (msg == WM_NCCREATE) {  //Это я тоже откуда-то скопировал
         printf("WndProc - WM_NCCREATE\n");
         //Получаем указатель на экземпляр нашего окна, который мы передали в функцию
         // CreateWindowEx
         self = (t_winapi_window*)LPCREATESTRUCT(lParam)->lpCreateParams;
#define GWL_USERDATA -21
         //И сохраняем в поле GWL_USERDATA
         SetWindowLongPtr(hWnd, GWL_USERDATA,
                          (long long)(LPCREATESTRUCT(lParam)->lpCreateParams));
         //Видимо какое-то поле, где можно привязав в hwnd сохранять что-то
      } else { // А если событие другое, то данные уже записаны
         self = (t_winapi_window*)GetWindowLongPtr(hWnd, GWL_USERDATA);
      }


      switch (msg) {
      case WM_DESTROY: //Закрытие окна
         printf("WndProc - WM_DESTROY\n");
         PostQuitMessage(0);
         break;

      case WM_KEYDOWN: //Нажатие клавиши
         self->_kd(wParam);
         break;

      case WM_KEYUP:   //Отпускание главиши
         self->_ku(wParam);
         break;


#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
         // Это макрост из windowssx.h, но чтобы не было лишней зависимости я сюда вытащил

      case WM_LBUTTONDOWN:
         self->_mm(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
         self->_kd(1);  //Левая клавиша мышки будет клавишей 1
         return 0;

      case WM_LBUTTONUP:
         self->_mm(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
         self->_ku(1);
         return 0;

      case WM_MOUSEMOVE:
         self->_mm(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
         return 0;

      case WM_RBUTTONDOWN:
         self->_mm(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
         self->_kd(2); //правая клавиша мышки будет клавишей 2
         return 0;

      case WM_RBUTTONUP:
         self->_mm(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
         self->_ku(2);
         return 0;
      case WM_ERASEBKGND: //Это чтобы перерисовывалось изображения, когда
         //мышкой двигаешь окно за пределами экрана
      case WM_PAINT:
         self->paint();
         InvalidateRect(self->hwnd, 0, 0);
         return 0; //Это событие стандартным образом не надо обрабатывать
      }
      return DefWindowProc(hWnd, msg, wParam, lParam);//Пусть оно само ненужные события обрабатывает

   }

   /// Для чего-то освобождаю ресурсы на выходе
   /// в целом ОС сама это сделает при закрытие процесса и это нужно только
   /// если в процессе работы программы окна создаются и удаляются, и они не должны загромождать
   /// ресурсы
   void stop(){
      DestroyWindow(hwnd);
      UnregisterClass(clname, 0);
      game.free();
   }
};

int main(){
   printf("start game");
   {
      t_winapi_window test;
      test.run();
   }
   printf("end game");
   return 0;
}
