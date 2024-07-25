#include "wsi-anonymizer.h"

char *get_app_name() {
#ifdef __linux__
    return "bin/wsi-anon.out";
#elif defined(__APPLE__) && defined(__MACH__)
    return "bin/wsi-anon.out";
#else
    return "exe\\wsi-anon.exe";
#endif
}

void print_help_message() {
    fprintf(stderr, "Usage: %s [FILE] [-OPTIONS]\n\n", get_app_name());
    fprintf(stderr, "OPTIONS:\n");
    fprintf(stderr, "-c     Only check file for vendor format and metadata\n");
    fprintf(stderr, "-n     Specify pseudo label name (e.g. -n \"labelname\")\n");
    fprintf(stderr, "-m     If flag is set, macro image will NOT be deleted\n");
    fprintf(stderr, "-i     If flag is set, anonymization will be done in-place\n");
    fprintf(stderr, "-u     If flag is set, tiff directory will NOT be unlinked\n\n");
    fprintf(stderr, "       Note: For file formats using JPEG compression this does not work currently.\n\n");
}

void print_metadata(struct wsi_data *wsi_data) {
    fprintf(stdout, "Vendor: %s\n", VENDOR_AND_FORMAT_STRINGS[wsi_data->format]);
    if (wsi_data->metadata_attributes->length != 0) {
        fprintf(stdout, "Metadata found:\n");
        for (size_t metadata_id = 0; metadata_id < wsi_data->metadata_attributes->length; metadata_id++) {
            fprintf(stdout, "%45s %s\n", wsi_data->metadata_attributes->attributes[metadata_id]->key,
                    wsi_data->metadata_attributes->attributes[metadata_id]->value);
        }
    } else {
        fprintf(stdout, "No metadata found.\n");
    }
}

int32_t main(int32_t argc, char *argv[]) {
    bool only_check = false;
    bool keep_macro_image = false;
    bool disable_unlinking = false;
    bool do_inplace = false;
    const char *filename = NULL;
    const char *new_label_name = NULL;

    if (argv[1] == NULL) {
        fprintf(stderr, "No filename specified.\n\n");
        print_help_message();
        exit(EXIT_FAILURE);
    }

    filename = argv[1];

    if (strcmp(filename, "-h\0") == 0 || strcmp(filename, "--help\0") == 0) {
        print_help_message();
        exit(EXIT_FAILURE);
    }

    for (int32_t optind = 2; optind < argc; optind++) {
        if (argv[optind][0] == '-') {
            switch (argv[optind][1]) {
            case 'c': {
                only_check = true;
                break;
            }
            case 'i': {
                do_inplace = true;
                break;
            }
            case 'n': {
                new_label_name = argv[optind + 1];
                break;
            }
            case 'u': {
                disable_unlinking = true;
                break;
            }
            case 'm': {
                keep_macro_image = true;
                break;
            }
            case 'h': {
                print_help_message();
                exit(EXIT_FAILURE);
            }
            default: {
                fprintf(stderr, "Invalid arguments.\n");
                print_help_message();
                exit(EXIT_FAILURE);
            }
            }
        }
    }

    if (only_check) {
        if (filename != NULL) {
            struct wsi_data *wsi_data = get_wsi_data(filename);
            // if format is supported and valid
            if (wsi_data->metadata_attributes != NULL) {
                // TODO: print out the rest of information (label and macro dims if available)
                print_metadata(wsi_data);
            }
            // handles invalid or unsupported file formats
            else {
                fprintf(stderr, "Error with %s format\n", VENDOR_AND_FORMAT_STRINGS[wsi_data->format]);
                exit(EXIT_FAILURE);
            }
            free_wsi_data(wsi_data);
            exit(EXIT_SUCCESS);
        } else {
            fprintf(stderr, "No filename to check for vendor selected.\n");
            exit(EXIT_FAILURE);
        }
    } else {
        if (filename != NULL) {
            if (new_label_name != NULL) {
                anonymize_wsi(filename, new_label_name, keep_macro_image, disable_unlinking, do_inplace);
            } else {
                // TODO: new file name (old_filename + tag)
                anonymize_wsi(filename, "_anonymized_wsi", keep_macro_image, disable_unlinking, do_inplace);
            }
            fprintf(stdout, "Done.\n");
            exit(EXIT_SUCCESS);
        } else {
            fprintf(stderr, "No file for anonymization selected.\n");
            exit(EXIT_FAILURE);
        }
    }
}
