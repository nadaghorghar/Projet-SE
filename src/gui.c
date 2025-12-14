
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>
#include "gui.h"
#include "process.h"


static Process *g_processes = NULL;
static int g_process_count = 0;
static GtkWidget *g_drawing_area = NULL;
static int selected_algo = 0;
static int rr_quantum = 2;


typedef struct {
    char filename[200];
    char name[200];
    char display_name[200];
    int needs_quantum;
} AlgoInfo;

static AlgoInfo *g_algos = NULL;
static int g_algo_count = 0;


#define BG_PRIMARY      0.09, 0.09, 0.11
#define BG_SECONDARY    0.13, 0.14, 0.17
#define BG_CARD         0.16, 0.17, 0.21
#define TEXT_PRIMARY    0.95, 0.96, 0.98
#define TEXT_SECONDARY  0.65, 0.68, 0.75
#define ACCENT_BLUE     0.34, 0.64, 0.98
#define ACCENT_SUCCESS  0.32, 0.84, 0.61
#define ACCENT_WARNING  0.98, 0.69, 0.31
#define ACCENT_PURPLE   0.62, 0.51, 0.98
#define GRID_COLOR      0.22, 0.23, 0.28

static double process_colors[][3] = {
    {0.34, 0.64, 0.98}, {0.62, 0.51, 0.98}, {0.32, 0.84, 0.61}, {0.98, 0.69, 0.31},
    {0.96, 0.45, 0.61}, {0.31, 0.87, 0.91}, {0.95, 0.58, 0.31}, {0.51, 0.78, 0.38},
};

static void show_results_window(GtkWidget *parent_window);

static void load_algorithms() {
    DIR *dir = opendir("politiques");
    if (!dir) {
        printf("Erreur : impossible d'ouvrir le dossier 'politiques'\n");
        exit(1);
    }

    g_algos = malloc(sizeof(AlgoInfo) * 50);
    g_algo_count = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        int len = strlen(entry->d_name);
        if (len < 3 || strcmp(entry->d_name + len - 2, ".c") != 0)
            continue;

        strcpy(g_algos[g_algo_count].filename, entry->d_name);
        
        strcpy(g_algos[g_algo_count].name, entry->d_name);
        g_algos[g_algo_count].name[len - 2] = '\0';
        
        if (strcmp(g_algos[g_algo_count].name, "fifo") == 0) {
            strcpy(g_algos[g_algo_count].display_name, "  FIFO");
        } else if (strcmp(g_algos[g_algo_count].name, "round_robin") == 0) {
            strcpy(g_algos[g_algo_count].display_name, "  Round Robin");
            g_algos[g_algo_count].needs_quantum = 1;
        } else if (strcmp(g_algos[g_algo_count].name, "priorite") == 0) {
            strcpy(g_algos[g_algo_count].display_name, " Priorité");
        } else if (strcmp(g_algos[g_algo_count].name, "multi_level") == 0) {
            strcpy(g_algos[g_algo_count].display_name, " Multi Level dynamique");
        } else if (strcmp(g_algos[g_algo_count].name, "multi_level_static") == 0) {
            strcpy(g_algos[g_algo_count].display_name, "  Multi Level Static");
        } else {
            char safe_name[180];
            strncpy(safe_name, g_algos[g_algo_count].name, sizeof(safe_name) - 1);
            safe_name[sizeof(safe_name) - 1] = '\0';
            snprintf(g_algos[g_algo_count].display_name, 200, "⚙️  %s", safe_name);
        }
        
        g_algo_count++;
    }

    closedir(dir);
    
    printf("✓ %d algorithmes détectés dans le dossier 'politiques'\n", g_algo_count);
}

static gboolean on_draw_gantt(GtkWidget *widget, cairo_t *cr, gpointer data) {
    (void)data;
    if (!current_result || !current_result->processes) return FALSE;
    
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);
    
    cairo_pattern_t *bg_gradient = cairo_pattern_create_linear(0, 0, 0, height);
    cairo_pattern_add_color_stop_rgb(bg_gradient, 0, BG_PRIMARY);
    cairo_pattern_add_color_stop_rgb(bg_gradient, 1, BG_SECONDARY);
    cairo_set_source(cr, bg_gradient);
    cairo_paint(cr);
    cairo_pattern_destroy(bg_gradient);
    
    int margin_left = 100, margin_top = 70, margin_right = 60, margin_bottom = 90;
    int chart_width = width - margin_left - margin_right;
    int chart_height = height - margin_top - margin_bottom;
    
    if (chart_width <= 0 || chart_height <= 0) return FALSE;
    
    int row_height = chart_height / (current_result->process_count + 1);
    if (row_height > 70) row_height = 70;
    if (row_height < 25) row_height = 25;
    
    cairo_set_source_rgb(cr, TEXT_PRIMARY);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 24);
    cairo_move_to(cr, margin_left, 40);
    char title[100];
    sprintf(title, "Diagramme de Gantt — %s", current_result->algo_name);
    cairo_show_text(cr, title);
    
    cairo_set_source_rgb(cr, ACCENT_BLUE);
    cairo_set_font_size(cr, 12);
    cairo_move_to(cr, margin_left, 58);
    cairo_show_text(cr, "Visualisation temporelle de l'exécution des processus");
    
    float scale = (float)chart_width / current_result->timeline_len;
    if (scale < 1.0) scale = 1.0;
    
    cairo_set_line_width(cr, 1);
    int step = (current_result->timeline_len > 50) ? 5 : 1;
    if (current_result->timeline_len > 100) step = 10;
    
    for (int t = 0; t <= current_result->timeline_len; t += step) {
        float x = margin_left + t * scale;
        
        cairo_set_source_rgba(cr, GRID_COLOR, 0.4);
        cairo_move_to(cr, x, margin_top);
        cairo_line_to(cr, x, margin_top + (current_result->process_count + 1) * row_height);
        cairo_stroke(cr);
        
        cairo_set_source_rgb(cr, TEXT_SECONDARY);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 11);
        char time_label[10];
        snprintf(time_label, sizeof(time_label), "%d", t);
        cairo_text_extents_t extents;
        cairo_text_extents(cr, time_label, &extents);
        cairo_move_to(cr, x - extents.width/2, height - margin_bottom + 25);
        cairo_show_text(cr, time_label);
    }
    
    cairo_set_source_rgb(cr, TEXT_PRIMARY);
    cairo_set_font_size(cr, 13);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_move_to(cr, width/2 - 30, height - margin_bottom + 50);
    cairo_show_text(cr, "Temps (unités)");
    
    for (int i = 0; i < current_result->process_count; i++) {
        int y = margin_top + i * row_height;
        
        if (i % 2 == 0) {
            cairo_set_source_rgba(cr, BG_CARD, 0.3);
            cairo_rectangle(cr, margin_left, y, chart_width, row_height);
            cairo_fill(cr);
        }
        
        cairo_set_source_rgb(cr, BG_CARD);
        cairo_rectangle(cr, 15, y + row_height/2 - 12, 70, 24);
        cairo_fill(cr);
        
        cairo_set_source_rgb(cr, TEXT_PRIMARY);
        cairo_set_font_size(cr, 13);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_move_to(cr, 25, y + row_height / 2 + 5);
        cairo_show_text(cr, current_result->processes[i].name);
        
        cairo_set_source_rgba(cr, GRID_COLOR, 0.6);
        cairo_set_line_width(cr, 1.5);
        cairo_move_to(cr, margin_left, y + row_height);
        cairo_line_to(cr, margin_left + chart_width, y + row_height);
        cairo_stroke(cr);
    }
    
    for (int t = 0; t < current_result->timeline_len; t++) {
        int proc_idx = current_result->timeline[t];
        
        if (proc_idx >= 0 && proc_idx < current_result->process_count) {
            float x = margin_left + t * scale;
            int y = margin_top + proc_idx * row_height;
            int color_idx = proc_idx % 8;
            
            cairo_set_source_rgba(cr, 0, 0, 0, 0.2);
            cairo_rectangle(cr, x + 2, y + 8, scale, row_height - 12);
            cairo_fill(cr);
            
            cairo_pattern_t *pattern = cairo_pattern_create_linear(x, y, x, y + row_height);
            cairo_pattern_add_color_stop_rgb(pattern, 0,
                process_colors[color_idx][0],
                process_colors[color_idx][1],
                process_colors[color_idx][2]);
            cairo_pattern_add_color_stop_rgb(pattern, 1,
                process_colors[color_idx][0] * 0.8,
                process_colors[color_idx][1] * 0.8,
                process_colors[color_idx][2] * 0.8);
            cairo_set_source(cr, pattern);
            cairo_rectangle(cr, x, y + 6, scale, row_height - 12);
            cairo_fill_preserve(cr);
            cairo_pattern_destroy(pattern);
            
            cairo_set_source_rgba(cr, 1, 1, 1, 0.15);
            cairo_set_line_width(cr, 1.5);
            cairo_stroke(cr);
        }
    }
    
    int legend_x = width - 200;
    int legend_y = 20;
    
    cairo_set_source_rgba(cr, 0, 0, 0, 0.3);
    cairo_rectangle(cr, legend_x + 2, legend_y + 2, 180, 60);
    cairo_fill(cr);
    
    cairo_set_source_rgb(cr, BG_CARD);
    cairo_rectangle(cr, legend_x, legend_y, 180, 60);
    cairo_fill(cr);
    
    cairo_set_source_rgba(cr, GRID_COLOR, 0.8);
    cairo_set_line_width(cr, 1);
    cairo_rectangle(cr, legend_x, legend_y, 180, 60);
    cairo_stroke(cr);
    
    cairo_set_source_rgb(cr, TEXT_PRIMARY);
    cairo_set_font_size(cr, 12);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_move_to(cr, legend_x + 15, legend_y + 25);
    cairo_show_text(cr, "Légende");
    
    cairo_set_source_rgb(cr, ACCENT_WARNING);
    cairo_rectangle(cr, legend_x + 15, legend_y + 35, 12, 12);
    cairo_fill(cr);
    
    cairo_set_source_rgb(cr, TEXT_SECONDARY);
    cairo_set_font_size(cr, 11);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_move_to(cr, legend_x + 32, legend_y + 45);
    cairo_show_text(cr, "CPU Inactif (IDLE)");
    
    return FALSE;
}

static GtkWidget* create_timeline_view() {
    if (!current_result || !current_result->processes) 
        return gtk_label_new("Erreur: pas de résultats");
    
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_margin_start(vbox, 30);
    gtk_widget_set_margin_end(vbox, 30);
    gtk_widget_set_margin_top(vbox, 30);
    gtk_widget_set_margin_bottom(vbox, 30);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title),
        "<span font='22' weight='bold' foreground='#57A4FA'>"
        "📋 Chronologie d'Exécution</span>");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 5);
    
    GtkWidget *subtitle = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(subtitle),
        "<span font='11' foreground='#A6AEC0'>"
        "Séquence détaillée des transitions entre processus</span>");
    gtk_widget_set_halign(subtitle, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), subtitle, FALSE, FALSE, 0);
    
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 10);
    
    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
    
    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_provider,
        "textview { "
        "  background: linear-gradient(135deg, #212329 0%, #292B35 100%);"
        "  color: #F2F5F9; "
        "  font-family: monospace; "
        "  font-size: 11pt; "
        "  padding: 20px; "
        "}", -1, NULL);
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(text_view),
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    
    char *timeline_text = malloc(50000);
    if (!timeline_text) return scrolled;
    timeline_text[0] = '\0';
    
    int items_per_line = 6;
    
    for (int t = 0; t < current_result->timeline_len; t++) {
        char entry[150];
        
        if (current_result->timeline[t] == -1) {
            snprintf(entry, sizeof(entry), "[IDLE:%d→%d]  ", t, t+1);
        } else {
            int proc_idx = current_result->timeline[t];
            snprintf(entry, sizeof(entry), "[%s:%d→%d]  ", 
                    current_result->processes[proc_idx].name, t, t+1);
        }
        
        strcat(timeline_text, entry);
        
        if ((t + 1) % items_per_line == 0 && t < current_result->timeline_len - 1) {
            strcat(timeline_text, "\n");
        }
    }
    
    gtk_text_buffer_set_text(buffer, timeline_text, -1);
    free(timeline_text);
    
    GtkWidget *text_scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(text_scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(text_scrolled, -1, 350);
    gtk_container_add(GTK_CONTAINER(text_scrolled), text_view);
    
    gtk_box_pack_start(GTK_BOX(vbox), text_scrolled, TRUE, TRUE, 0);
    
    int idle_count = 0;
    for (int t = 0; t < current_result->timeline_len; t++) {
        if (current_result->timeline[t] == -1) idle_count++;
    }
    
    float cpu_utilization = ((float)(current_result->timeline_len - idle_count) / current_result->timeline_len) * 100.0;
    
    GtkWidget *stats_label = gtk_label_new(NULL);
    char stats[800];
    snprintf(stats, sizeof(stats),
        "<span font='12'>\n"
        "<span foreground='#52D69C'><b>📊 Métriques de Performance</b></span>\n\n"
        "<span foreground='#A6AEC0'>"
        "   <b>Durée totale:</b></span> <span foreground='#F2F5F9'>%d unités</span>\n"
        "<span foreground='#A6AEC0'>"
        "   <b>CPU actif:</b></span> <span foreground='#52D69C'>%d unités</span>\n"
        "<span foreground='#A6AEC0'>"
        "   <b>CPU inactif:</b></span> <span foreground='#FAB04F'>%d unités</span>\n"
        "<span foreground='#A6AEC0'>"
        "   <b>Utilisation CPU:</b></span> <span foreground='#57A4FA' weight='bold'>%.1f%%</span>\n"
        "</span>",
        current_result->timeline_len,
        current_result->timeline_len - idle_count,
        idle_count,
        cpu_utilization);
    
    gtk_label_set_markup(GTK_LABEL(stats_label), stats);
    gtk_widget_set_halign(stats_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), stats_label, FALSE, FALSE, 15);
    
    gtk_container_add(GTK_CONTAINER(scrolled), vbox);
    return scrolled;
}

static GtkWidget* create_stats_table() {
    if (!current_result || !current_result->processes) 
        return gtk_label_new("Erreur: pas de résultats");
    
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 20);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_widget_set_margin_start(grid, 30);
    gtk_widget_set_margin_end(grid, 30);
    gtk_widget_set_margin_top(grid, 30);
    gtk_widget_set_margin_bottom(grid, 30);
    
    const char *headers[] = {
        "Processus", "Arrivée", "Durée", "Début", "Fin", "Turnaround", "Attente"
    };
    
    for (int col = 0; col < 7; col++) {
        GtkWidget *label = gtk_label_new(NULL);
        char markup[200];
        snprintf(markup, sizeof(markup), 
                "<b><span foreground='#57A4FA' font='12'>%s</span></b>", headers[col]);
        gtk_label_set_markup(GTK_LABEL(label), markup);
        gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
        gtk_grid_attach(GTK_GRID(grid), label, col, 0, 1, 1);
    }
    
    Process *procs = current_result->processes;
    
    for (int i = 0; i < current_result->process_count; i++) {
        char text[250];
        GtkWidget *label;
        
        label = gtk_label_new(NULL);
        int color_idx = i % 8;
        snprintf(text, sizeof(text), 
                "<span foreground='#%02X%02X%02X' weight='bold' font='11'>● </span>"
                "<span foreground='#F2F5F9' font='11'>%s</span>",
                (int)(process_colors[color_idx][0] * 255),
                (int)(process_colors[color_idx][1] * 255),
                (int)(process_colors[color_idx][2] * 255),
                procs[i].name);
        gtk_label_set_markup(GTK_LABEL(label), text);
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(grid), label, 0, i + 1, 1, 1);
        
        for (int col = 1; col < 7; col++) {
            label = gtk_label_new(NULL);
            const char *color = (col >= 5) ? 
                (col == 5 ? "#52D69C" : "#FAB04F") : "#A6AEC0";
            snprintf(text, sizeof(text), 
                    "<span foreground='%s' font='11'>%d</span>", 
                    color, (col == 1) ? procs[i].arrival :
                          (col == 2) ? procs[i].duration :
                          (col == 3) ? current_result->start[i] :
                          (col == 4) ? current_result->end[i] :
                          (col == 5) ? current_result->turnaround[i] :
                                       current_result->wait[i]);
            gtk_label_set_markup(GTK_LABEL(label), text);
            gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
            gtk_grid_attach(GTK_GRID(grid), label, col, i + 1, 1, 1);
        }
    }
    
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_grid_attach(GTK_GRID(grid), sep, 0, current_result->process_count + 1, 7, 1);
    
    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), 
        "<b><span foreground='#9E82FA' font='12'>Moyennes</span></b>");
    gtk_grid_attach(GTK_GRID(grid), label, 0, current_result->process_count + 2, 5, 1);
    
    label = gtk_label_new(NULL);
    char markup[200];
    snprintf(markup, sizeof(markup), 
            "<b><span foreground='#52D69C' font='13'>%.2f</span></b>", 
            current_result->avg_turnaround);
    gtk_label_set_markup(GTK_LABEL(label), markup);
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(grid), label, 5, current_result->process_count + 2, 1, 1);
    
    label = gtk_label_new(NULL);
    snprintf(markup, sizeof(markup), 
            "<b><span foreground='#FAB04F' font='13'>%.2f</span></b>", 
            current_result->avg_wait);
    gtk_label_set_markup(GTK_LABEL(label), markup);
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(grid), label, 6, current_result->process_count + 2, 1, 1);
    
    gtk_container_add(GTK_CONTAINER(scrolled), grid);
    return scrolled;
}

static void show_results_window(GtkWidget *parent_window) {
    if (!current_result) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(parent_window),
            GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
            "Erreur lors de l'exécution de l'algorithme");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    GtkWidget *result_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    char window_title[100];
    sprintf(window_title, "Résultats — %s", current_result->algo_name);
    gtk_window_set_title(GTK_WINDOW(result_window), window_title);
    gtk_window_set_default_size(GTK_WINDOW(result_window), 1300, 750);
    gtk_window_set_position(GTK_WINDOW(result_window), GTK_WIN_POS_CENTER);
    
    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_provider,
        "window { background: linear-gradient(135deg, #171719 0%, #212329 100%); }"
        "box { background-color: transparent; }"
        "notebook { background-color: #171719; border: 1px solid #383A47; }"
        "notebook tab { background: #292B35; color: #A6AEC0; padding: 14px 24px; }"
        "notebook tab:checked { background: #57A4FA; color: #FFFFFF; }"
        "scrolledwindow { background-color: #212329; }"
        "grid { background-color: #212329; }", -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    
    GtkWidget *notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
    
    GtkWidget *timeline = create_timeline_view();
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), timeline, gtk_label_new("📋 Chronologie"));
    
    g_drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(g_drawing_area, 1200, 550);
    g_signal_connect(G_OBJECT(g_drawing_area), "draw", G_CALLBACK(on_draw_gantt), NULL);
    
    GtkWidget *gantt_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(gantt_scroll), g_drawing_area);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), gantt_scroll, gtk_label_new("📊 Gantt"));
    
    GtkWidget *stats = create_stats_table();
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), stats, gtk_label_new("📈 Statistiques"));
    
    gtk_container_add(GTK_CONTAINER(result_window), notebook);
    gtk_widget_show_all(result_window);
}

static void on_algo_selected(GtkWidget *widget, gpointer data) {
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
        selected_algo = GPOINTER_TO_INT(data);
    }
}

static void on_execute_algo(GtkWidget *widget, gpointer data) {
    (void)widget;
    GtkWidget *window = GTK_WIDGET(data);
    
    if (g_algos[selected_algo].needs_quantum) {
        GtkWidget *dialog = gtk_dialog_new_with_buttons(
            "Configuration Round Robin", GTK_WINDOW(window),
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            "Annuler", GTK_RESPONSE_CANCEL, "OK", GTK_RESPONSE_OK, NULL);
        
        GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        gtk_widget_set_margin_start(content, 30);
        gtk_widget_set_margin_end(content, 30);
        gtk_widget_set_margin_top(content, 25);
        gtk_widget_set_margin_bottom(content, 25);
        
        GtkWidget *label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(label), 
            "<span font='13' foreground='#F2F5F9'>Quantum (unités de temps) :</span>");
        gtk_box_pack_start(GTK_BOX(content), label, FALSE, FALSE, 10);
        
        GtkWidget *spin = gtk_spin_button_new_with_range(1, 100, 1);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), rr_quantum);
        gtk_box_pack_start(GTK_BOX(content), spin, FALSE, FALSE, 10);
        
        gtk_widget_show_all(dialog);
        
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        if (response == GTK_RESPONSE_OK) {
            rr_quantum = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin));
        }
        gtk_widget_destroy(dialog);
        
        if (response != GTK_RESPONSE_OK) return;
    }
    
    if (current_result) {
        if (current_result->processes) free(current_result->processes);
        free(current_result);
    }
    
    current_result = malloc(sizeof(SchedulingResult));
    if (!current_result) return;
    
    capture_mode = 1;
    
    char path[300];
    char lib[300];
    
    snprintf(path, sizeof(path), "politiques/%s", g_algos[selected_algo].filename);
    snprintf(lib, sizeof(lib), "politiques/%s.so", g_algos[selected_algo].filename);
    
    remove(lib);
    
    char cmd[800];
    snprintf(cmd, sizeof(cmd),
             "gcc -shared -fPIC -Isrc %s -o %s 2>/dev/null",
             path, lib);
    
    if (system(cmd) != 0) {
        printf("Erreur lors de la compilation de %s\n", g_algos[selected_algo].name);
        capture_mode = 0;
        return;
    }
    
    void *handle = dlopen(lib, RTLD_NOW);
    if (!handle) {
        printf("Erreur dlopen: %s\n", dlerror());
        capture_mode = 0;
        return;
    }
    
    printf("\n>>> Exécution de : %s\n", g_algos[selected_algo].name);
    
    if (g_algos[selected_algo].needs_quantum) {
        void (*rr_func)(Process*, int, int) = dlsym(handle, g_algos[selected_algo].name);
        if (!rr_func) {
            printf("Erreur dlsym: %s\n", dlerror());
            dlclose(handle);
            capture_mode = 0;
            return;
        }
        rr_func(g_processes, g_process_count, rr_quantum);
    } else {
        void (*alg_func)(Process*, int) = dlsym(handle, g_algos[selected_algo].name);
        if (!alg_func) {
            printf("Erreur dlsym: %s\n", dlerror());
            dlclose(handle);
            capture_mode = 0;
            return;
        }
        alg_func(g_processes, g_process_count);
    }
    
    dlclose(handle);
    capture_mode = 0;
    show_results_window(window);
}

void lancer_interface_gtk(Process procs[], int count) {
    g_processes = procs;
    g_process_count = count;
    
    load_algorithms();
    
    if (g_algo_count == 0) {
        printf("Erreur : aucun algorithme trouvé dans le dossier 'politiques'\n");
        exit(1);
    }
    
    gtk_init(NULL, NULL);
    
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Ordonnanceur de Processus");
    gtk_window_set_default_size(GTK_WINDOW(window), 950, 800);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    
    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_provider,
        "window { "
        "  background: linear-gradient(135deg, #0F0F11 0%, #1A1B1E 50%, #0F0F11 100%); "
        "}"
        "box { background-color: transparent; }"
        ".algo-card { "
        "  background: linear-gradient(135deg, #1E1F26 0%, #25262E 100%); "
        "  border: 1px solid #35363F; "
        "  border-radius: 12px; "
        "  padding: 18px; "
        "  margin: 8px; "
        "}"
        ".algo-card:hover { "
        "  background: linear-gradient(135deg, #252631 0%, #2C2D36 100%); "
        "  border-color: #57A4FA; "
        "  box-shadow: 0 4px 20px rgba(87, 164, 250, 0.15); "
        "}"
        "radiobutton { "
        "  color: #F2F5F9; "
        "  font-size: 13px; "
        "  font-weight: 600; "
        "}"
        "radiobutton:checked { color: #57A4FA; }"
        "button { "
        "  background: linear-gradient(135deg, #57A4FA 0%, #4891E6 100%); "
        "  color: #FFFFFF; "
        "  border: none; "
        "  border-radius: 14px; "
        "  padding: 18px 40px; "
        "  font-weight: 700; "
        "  font-size: 16px; "
        "  box-shadow: 0 8px 25px rgba(87, 164, 250, 0.4); "
        "}"
        "button:hover { "
        "  background: linear-gradient(135deg, #6BB3FF 0%, #57A4FA 100%); "
        "  box-shadow: 0 10px 30px rgba(87, 164, 250, 0.5); "
        "}"
        "label { color: #F2F5F9; }"
        "separator { background: #35363F; min-height: 1px; }",
        -1, NULL);
    
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    
    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    
    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(header_box, 50);
    gtk_widget_set_margin_end(header_box, 50);
    gtk_widget_set_margin_top(header_box, 45);
    gtk_widget_set_margin_bottom(header_box, 35);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title),
        "<span font='24' weight='bold' foreground='#57A4FA'>"
        "⚡ </span>"
        "<span font='28' weight='bold' foreground='#F2F5F9'>"
        "Ordonnanceur </span>"
        "<span font='28' weight='bold' foreground='#57A4FA'>"
        "de Processus</span>");
    gtk_box_pack_start(GTK_BOX(header_box), title, FALSE, FALSE, 0);
    
    GtkWidget *subtitle = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(subtitle),
        "<span font='12' foreground='#B0B5BA'>"
        "Simulation et analyse avancée d'algorithmes d'ordonnancement CPU"
        "</span>");
    gtk_box_pack_start(GTK_BOX(header_box), subtitle, FALSE, FALSE, 0);
    
    char info[512];
    snprintf(info, sizeof(info),
        "<span font='11' foreground='#8A8F95'>"
        "● </span>"
        "<span font='11' weight='600' foreground='#F2F5F9'>"
        "%d processus chargés </span>"
        "<span font='11' foreground='#57A4FA'>"
        "| </span>"
        "<span font='11' foreground='#8A8F95'>"
        "● </span>"
        "<span font='11' weight='600' foreground='#F2F5F9'>"
        "%d algorithmes disponibles</span>",
        count, g_algo_count);
    
    GtkWidget *info_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(info_label), info);
    gtk_box_pack_start(GTK_BOX(header_box), info_label, FALSE, FALSE, 5);
    
    gtk_box_pack_start(GTK_BOX(main_vbox), header_box, FALSE, FALSE, 0);
    
    GtkWidget *sep1 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(main_vbox), sep1, FALSE, FALSE, 0);
    
    GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_start(content_box, 50);
    gtk_widget_set_margin_end(content_box, 50);
    gtk_widget_set_margin_top(content_box, 30);
    gtk_widget_set_margin_bottom(content_box, 35);
    
    GtkWidget *algo_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(algo_title),
        "<span font='16' weight='bold' foreground='#F2F5F9'>"
        "Sélection de l'algorithme</span>");
    gtk_widget_set_halign(algo_title, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(content_box), algo_title, FALSE, FALSE, 5);
    
    GtkWidget *algo_subtitle = gtk_label_new(NULL);
    char subtitle_text[400];
    
    int fifo_index = 0;
    for (int i = 0; i < g_algo_count; i++) {
        if (strcmp(g_algos[i].name, "fifo") == 0) {
            fifo_index = i;
            break;
        }
    }
    
    char safe_display_name[100];
    strncpy(safe_display_name, g_algos[fifo_index].display_name, sizeof(safe_display_name) - 1);
    safe_display_name[sizeof(safe_display_name) - 1] = '\0';
    
    snprintf(subtitle_text, sizeof(subtitle_text),
        "<span font='11' foreground='#B0B5BA'>"
        "Choisissez un algorithme d'ordonnancement ci-dessous (%s par défaut)"
        "</span>", safe_display_name);
    
    gtk_label_set_markup(GTK_LABEL(algo_subtitle), subtitle_text);
    gtk_widget_set_halign(algo_subtitle, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(content_box), algo_subtitle, FALSE, FALSE, 0);
    
    GtkWidget *algos_grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(algos_grid), 15);
    gtk_grid_set_row_spacing(GTK_GRID(algos_grid), 15);
    gtk_widget_set_margin_top(algos_grid, 20);
    
    GSList *radio_group = NULL;
    
    for (int i = 0; i < g_algo_count; i++) {
        GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        GtkStyleContext *context = gtk_widget_get_style_context(card);
        gtk_style_context_add_class(context, "algo-card");
        
        GtkWidget *radio = gtk_radio_button_new_with_label(radio_group, g_algos[i].display_name);
        radio_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio));
        
        if (i == fifo_index) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), TRUE);
            selected_algo = i;
        }
        
        g_signal_connect(radio, "toggled", G_CALLBACK(on_algo_selected), GINT_TO_POINTER(i));
        
        const char *description = "Algorithme d'ordonnancement";
        if (strcmp(g_algos[i].name, "fifo") == 0) {
            description = "Premier arrivé, premier servi";
        } else if (strcmp(g_algos[i].name, "round_robin") == 0) {
            description = "Temps de CPU partagé équitablement";
        } else if (strcmp(g_algos[i].name, "priorite") == 0) {
            description = "Exécution par ordre de priorité";
        } else if (strcmp(g_algos[i].name, "multi_level") == 0) {
            description = "";
        } else if (strcmp(g_algos[i].name, "multi_level_static") == 0) {
            description = "";
        }
        
        GtkWidget *desc_label = gtk_label_new(NULL);
        char desc_markup[300];
        snprintf(desc_markup, sizeof(desc_markup),
            "<span font='10' foreground='#8A8F95'>%s</span>", description);
        gtk_label_set_markup(GTK_LABEL(desc_label), desc_markup);
        gtk_widget_set_halign(desc_label, GTK_ALIGN_START);
        gtk_widget_set_margin_start(desc_label, 25);
        
        gtk_box_pack_start(GTK_BOX(card), radio, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(card), desc_label, FALSE, FALSE, 0);
        
        int row = i / 2;
        int col = i % 2;
        gtk_grid_attach(GTK_GRID(algos_grid), card, col, row, 1, 1);
        gtk_widget_set_hexpand(card, TRUE);
    }
    
    gtk_box_pack_start(GTK_BOX(content_box), algos_grid, FALSE, FALSE, 0);
    
    GtkWidget *sep2 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_margin_top(sep2, 15);
    gtk_widget_set_margin_bottom(sep2, 15);
    gtk_box_pack_start(GTK_BOX(content_box), sep2, FALSE, FALSE, 0);
    
    GtkWidget *execute_btn = gtk_button_new_with_label("▶ Exécuter l'algorithme");
    gtk_widget_set_size_request(execute_btn, 400, 60);
    gtk_widget_set_halign(execute_btn, GTK_ALIGN_CENTER);
    g_signal_connect(execute_btn, "clicked", G_CALLBACK(on_execute_algo), window);
    gtk_box_pack_start(GTK_BOX(content_box), execute_btn, FALSE, FALSE, 10);
    
    gtk_box_pack_start(GTK_BOX(main_vbox), content_box, TRUE, TRUE, 0);
    
    gtk_container_add(GTK_CONTAINER(scrolled_window), main_vbox);
    
    gtk_container_add(GTK_CONTAINER(window), scrolled_window);
    
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    gtk_widget_show_all(window);
    gtk_main();
    
    if (current_result) {
        if (current_result->processes) free(current_result->processes);
        free(current_result);
    }
    if (g_algos) {
        free(g_algos);
    }
}
