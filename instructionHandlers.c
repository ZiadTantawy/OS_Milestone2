#include <stdio.h>
#include "memory.h"
#include "pcb.h"
#include "mutex.h"
#include "scheduler.h"
#include "queue.h"
#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "gui.h"

// Declare widgets as external since it's defined in gui.c
extern AppWidgets widgets;

void handleAssign(const char *params, int pcbMemoryEndIndex)
{
    char varName[50], value[50];
    sscanf(params, "%s %s", varName, value);

    // Search for the variable memory slot
    for (int i = pcbMemoryEndIndex - 3; i < pcbMemoryEndIndex - 1; i++)
    {
        if (strcmp(memory[i].name, varName) == 0 || strcmp(memory[i].name, "") == 0)
        {
            if (strcmp(value, "input") == 0)
            {
                // GUI-based input handling
                GtkWidget *dialog;
                GtkWidget *content_area;
                GtkWidget *entry;
                GtkWidget *label;

                // Create dialog
                dialog = gtk_dialog_new_with_buttons("Input Required",
                                                     GTK_WINDOW(widgets.window),
                                                     GTK_DIALOG_MODAL,
                                                     "_OK", GTK_RESPONSE_ACCEPT,
                                                     NULL);

                // Create content area with label and entry
                content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
                label = gtk_label_new_with_mnemonic(g_strdup_printf("Please enter a value for %s:", varName));
                entry = gtk_entry_new();

                gtk_container_add(GTK_CONTAINER(content_area), label);
                gtk_container_add(GTK_CONTAINER(content_area), entry);
                gtk_widget_show_all(dialog);

                // Run dialog and get input
                if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
                {
                    strcpy(value, gtk_entry_get_text(GTK_ENTRY(entry)));
                    add_log_message(g_strdup_printf("User entered: %s", value));
                }

                gtk_widget_destroy(dialog);
            }

            strcpy(memory[i].name, varName);
            strcpy(memory[i].data, value);
            return;
        }
    }
    add_log_message(("Variable %s not found or no space!\n", varName));
}

void handlePrint(const char *varName, int pcbMemoryEndIndex)
{
    for (int i = pcbMemoryEndIndex - 3; i < pcbMemoryEndIndex - 1; i++)
    {
        if (strcmp(memory[i].name, varName) == 0)
        {
            add_log_message(("%s = %s\n", varName, memory[i].data));
            return;
        }
    }
    add_log_message("Variable %s not found!\n", varName);
}

void handleSemWait(const char *resourceName, PCB *pcb)
{
    if (strcmp(resourceName, "userInput") == 0)
    {
        semWait(&userInputMutex, pcb);
    }
    else if (strcmp(resourceName, "userOutput") == 0)
    {
        semWait(&userOutputMutex, pcb);
    }
    else if (strcmp(resourceName, "file") == 0)
    {
        semWait(&fileMutex, pcb);
    }
    else
    {
        add_log_message("Unknown resource in semWait: %s\n", resourceName);
    }

    // If the process got BLOCKED, update memory too
    if (pcb->state == BLOCKED)
    {
        writeMemory(pcb->memoryEnd + 2, "PCB_state", "BLOCKED");
    }
}

void handleSemSignal(const char *resourceName)
{
    if (strcmp(resourceName, "userInput") == 0)
    {
        semSignal(&userInputMutex);
    }
    else if (strcmp(resourceName, "userOutput") == 0)
    {
        semSignal(&userOutputMutex);
    }
    else if (strcmp(resourceName, "file") == 0)
    {
        semSignal(&fileMutex);
    }
    else
    {
        add_log_message("Unknown resource in semSignal: %s\n", resourceName);
    }
}

void handlePrintFromTo(const char *params, int pcbEndIndex)
{
    char var1[50], var2[50];
    sscanf(params, "%s %s", var1, var2);
    int a = -1, b = -1;

    for (int i = pcbEndIndex - 3; i < pcbEndIndex - 1; i++)
    {
        if (strcmp(memory[i].name, var1) == 0)
        {
            a = atoi(memory[i].data);
        }
        if (strcmp(memory[i].name, var2) == 0)
        {
            b = atoi(memory[i].data);
        }
    }

    if (a == -1 || b == -1)
    {
        add_log_message("Variables for printFromTo not found!\n");
        return;
    }

    for (int i = a; i <= b; i++)
    {
        add_log_message("%d ", i);
    }
}

void handleWriteFile(const char *params)
{
    char filename[50], data[100];
    sscanf(params, "%s %[^\n]", filename, data);

    FILE *fp = fopen(filename, "w");
    if (fp == NULL)
    {
        add_log_message("Error opening file %s for writing.\n", filename);
        return;
    }
    fprintf(fp, "%s", data);
    fclose(fp);

    add_log_message("Data written to file %s successfully.\n", filename);
}

void handleReadFile(const char *filename, int pcbMemoryEndIndex)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        add_log_message("Error opening file %s for reading.\n", filename);
        return;
    }

    char data[100];
    fscanf(fp, "%s", data);
    fclose(fp);

    // Save into a free variable slot
    for (int i = pcbMemoryEndIndex - 3; i < pcbMemoryEndIndex - 1; i++)
    {
        if (strcmp(memory[i].name, "") == 0)
        {
            strcpy(memory[i].name, "fileData");
            strcpy(memory[i].data, data);
            add_log_message("File data saved into memory.\n");
            return;
        }
    }
    add_log_message("No free space to save file data.\n");
}