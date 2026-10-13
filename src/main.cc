


int main(int argc, char **argv) {


    RegistryDefaultModule();

    if(!RegistryPlugins()) {
        return -1;
    }

    if(!BuildGraph()) {
        return -1;
    }

    while(true) {

    }
}