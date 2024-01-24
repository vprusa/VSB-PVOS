
Upravte SSL server a klienta tak, aby byl schopen připojit více klientů současně a server bude přijatá data vypisovat. Pro kontrolu by bylo vhodné, aby server klientovi zpět potvrdil, kolik bajtů přijat.
Vytvořte verzi s vlákny a procesy (za fork se zavolá kód vlákna).
Zkuste obsluhu a příjem více klientů jen v jednom procesu s pomocí select nebo poll. Seznam deskriptorů bude potřeba rozšířit o strukturu SSL.
