/*
    Copyright (C) 2015 Tomas Flouri, Sarah Lutteropp

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Contact: Tomas Flouri <Tomas.Flouri@h-its.org>,
    Heidelberg Institute for Theoretical Studies,
    Schloss-Wolfsbrunnenweg 35, D-69118 Heidelberg, Germany
*/

#include "delimit.h"


static char line[LINEALLOC];

static double originx = 133;

static int xtics = 10;

static long canvas_x1 = 130;
static long canvas_x2 = 730;
static long canvas_y1 = 10;
static long canvas_y2 = 360;
static double radius = 2;

static void svg_header(FILE * svg_fp)
{

  fprintf(svg_fp,"<svg class=\"graph\" version=\"1.1\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns=\"http://www.w3.org/2000/svg\">");
  fprintf(svg_fp,"<style type=\"text/css\">");
  fprintf(svg_fp,"<![CDATA[\n"
                 "svg.graph {\n"
                 " height: 500px;\n"
                 " width: 800px;\n"
                 " background: #b2e9e4;\n"
                 "}\n\n"
                 "svg.graph .grid {\n"
                 " stroke: white;\n"
                 " stroke-dasharray: 1 2;\n"
                 " stroke-width: 1;\n"
                 "}\n\n"
                 "svg.graph .points {\n"
                 " stroke: white;\n"
                 " stroke-width: 3;\n"
                 "}\n\n"
                 "svg.graph .first_set {\n"
                 " fill: #00554d;\n"
                 "}\n\n"
                 "svg.graph .first_set_bar {\n"
                 " fill: #00554d;\n"
                 " stroke: #000000;\n"
                 "}\n\n"
                 "svg.graph .surfaces {\n"
                 " fill-opacity: 0.5;\n"
                 "}\n\n"
                 "svg.graph .grid.double {\n"
                 " stroke-opacity: 0.4;\n"
                 "}\n\n"
                 "svg.graph .labels {\n"
                 " font-family: Arial;\n"
                 " font-size: 14px;\n"
                 " kerning: 1;\n"
                 "}\n"
                 "svg.graph .labels.x-labels {\n"
                 " text-anchor: end;\n"
                 "}\n"
                 "svg.graph .labels.y-labels {\n"
                 " text-anchor: end;\n"
                 "}\n"
                 "]]>\n</style>\n");

  /* print axes */
  fprintf(svg_fp, "<g class=\"grid x-grid\" id=\"xGrid\">\n"
                  "  <line x1=\"130\" x2=\"130\" y1=\"10\" y2=\"380\"></line>\n"
                  "  <line x1=\"190\" x2=\"190\" y1=\"10\" y2=\"380\"></line>\n"
                  "  <line x1=\"250\" x2=\"250\" y1=\"10\" y2=\"380\"></line>\n"
                  "  <line x1=\"310\" x2=\"310\" y1=\"10\" y2=\"380\"></line>\n"
                  "  <line x1=\"370\" x2=\"370\" y1=\"10\" y2=\"380\"></line>\n"
                  "  <line x1=\"430\" x2=\"430\" y1=\"10\" y2=\"380\"></line>\n"
                  "  <line x1=\"490\" x2=\"490\" y1=\"10\" y2=\"380\"></line>\n"
                  "  <line x1=\"550\" x2=\"550\" y1=\"10\" y2=\"380\"></line>\n"
                  "  <line x1=\"610\" x2=\"610\" y1=\"10\" y2=\"380\"></line>\n"
                  "  <line x1=\"670\" x2=\"670\" y1=\"10\" y2=\"380\"></line>\n"
                  "  <line x1=\"730\" x2=\"730\" y1=\"10\" y2=\"380\"></line>\n"
                  "</g>\n");
                  
  fprintf(svg_fp, "<g class=\"grid y-grid\" id=\"yGrid\">\n"
                  "  <line x1=\"103\" x2=\"730\" y1=\"10\" y2=\"10\"></line>\n"
                  "  <line x1=\"103\" x2=\"730\" y1=\"68\" y2=\"68\"></line>\n"
                  "  <line x1=\"103\" x2=\"730\" y1=\"126\" y2=\"126\"></line>\n"
                  "  <line x1=\"103\" x2=\"730\" y1=\"185\" y2=\"185\"></line>\n"
                  "  <line x1=\"103\" x2=\"730\" y1=\"243\" y2=\"243\"></line>\n"
                  "  <line x1=\"103\" x2=\"730\" y1=\"301\" y2=\"301\"></line>\n"
                  "  <line x1=\"103\" x2=\"730\" y1=\"360\" y2=\"360\"></line>\n"
                  "</g>\n");
}

static void out_svg(FILE * svg_fp, double min_logl, double max_logl)
{
  
  double scale = (max_logl - min_logl) * 1.1;
  fprintf(svg_fp, "<g class=\"surfaces\">\n");
  
  /* open data points file */
  char * filename = (char *)xmalloc((strlen(opt_outfile)+10)*sizeof(char));
  strcpy(filename,opt_outfile);
  strcat(filename,".log");
  FILE * fp = xopen(filename,"r");
  free(filename);

  /* read and print data points to svg */
  int i = 0;
  while (fgets(line,LINEALLOC,fp))
  {
    double x,y;
    double logl;
    int species;

    sscanf(line,"%lf,%d\n",&logl,&species);

    /* compute x point */
    x = ((i*opt_bayes_sample)/(double)(opt_bayes_runs-opt_bayes_burnin)) *
        (canvas_x2 - canvas_x1) + canvas_x1;

    /* compute y point */
    y = (1 - (logl-min_logl)/scale) *
        (canvas_y2-canvas_y1) + 
        canvas_y1;

    /* print point */
    fprintf(svg_fp,
            "<circle cx=\"%f\" cy=\"%f\" r=\"%f\" fill=\"%s\" "
            "stroke=\"%s\" />\n",
            x, y, radius, "#000000", "#000000");

    ++i;

  }
  fclose(fp);
  fprintf(svg_fp, "</g>\n");
}
  
void svg_footer(FILE * svg_fp, double min_logl, double max_logl)
{
  double scale = (max_logl - min_logl) * 1.1;
  int i;

  /* define class usage */
  fprintf(svg_fp,"<use class=\"grid double\" xlink:href=\"#xGrid\" style=\"\"></use>\n");
  fprintf(svg_fp,"<use class=\"grid double\" xlink:href=\"#yGrid\" style=\"\"></use>\n");

  /* x labels */
  fprintf(svg_fp, "<g class=\"labels x-labels\">\n");
  fprintf(svg_fp, "<text transform=\"translate(%f,400)rotate(270)\">%ld</text>\n",
                  originx,
                  opt_bayes_burnin);
  for (i = 0; i < xtics; ++i)
  {
    fprintf(svg_fp,
            "<text transform=\"translate(%f,400)rotate(270)\">%ld</text>\n",
              originx + (i+1)*((canvas_x2 - canvas_x1)/(double)xtics), 
              (long)((i+1)*((opt_bayes_runs-opt_bayes_burnin)/(double)xtics)) +
                    opt_bayes_burnin);
  }
  fprintf(svg_fp, "</g>\n");

  /* y labels */
  fprintf(svg_fp, "<g class=\"labels y-labels\">\n");
  fprintf(svg_fp, " <text x=\"100\" y=\"15\">%.3f</text>\n",  min_logl + scale);
  fprintf(svg_fp, " <text x=\"100\" y=\"73\">%.3f</text>\n",  min_logl + 5*(scale)/6);
  fprintf(svg_fp, " <text x=\"100\" y=\"131\">%.3f</text>\n", min_logl + 4*(scale)/6);
  fprintf(svg_fp, " <text x=\"100\" y=\"190\">%.3f</text>\n", min_logl + 3*(scale)/6);
  fprintf(svg_fp, " <text x=\"100\" y=\"248\">%.3f</text>\n", min_logl + 2*(scale)/6);
  fprintf(svg_fp, " <text x=\"100\" y=\"307\">%.3f</text>\n", min_logl + scale/6);
  fprintf(svg_fp, " <text x=\"100\" y=\"365\">%.3f</text>\n", min_logl);
  fprintf(svg_fp, "</g>\n");

  fprintf(svg_fp,"</svg>\n");
}

void svg_landscape(double bayes_min_logl, double bayes_max_logl)
{
  FILE * svg_fp = open_file_ext(".logl.svg");

  svg_header(svg_fp);
  out_svg(svg_fp, bayes_min_logl, bayes_max_logl);
  svg_footer(svg_fp, bayes_min_logl, bayes_max_logl);

  fclose(svg_fp);
}
