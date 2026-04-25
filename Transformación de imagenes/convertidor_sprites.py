import os
from PIL import Image
import struct

# --- CONFIGURACIÓN ---
# Asegúrate de que tus imágenes PNG estén en una carpeta llamada 'sprites_entrada'
CARPETA_ENTRADA = 'imagenes'
CARPETA_SALIDA = 'sprites_binarios'
ANCHO_SPRITE = 320  # Ajusta al tamaño que necesites
ALTO_SPRITE = 240

def convertir_a_rgb565(r, g, b):
    """
    Convierte colores de 8 bits (0-255) a formato RGB565 de 16 bits.
    R: 5 bits, G: 6 bits, B: 5 bits.
    """
    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F
    b5 = (b >> 3) & 0x1F
    
    # Empaquetamos en un entero de 16 bits (Big Endian para la TFT)
    return (r5 << 11) | (g6 << 5) | b5

def procesar_imagenes():
    # Crear carpeta de salida si no existe
    if not os.path.exists(CARPETA_SALIDA):
        os.makedirs(CARPETA_SALIDA)
        print(f"Carpeta '{CARPETA_SALIDA}' creada.")

    # Listar archivos PNG en la carpeta de entrada
    archivos = [f for f in os.listdir(CARPETA_ENTRADA) if f.endswith('.png')]
    
    if not archivos:
        print(f"No se encontraron archivos PNG en '{CARPETA_ENTRADA}'.")
        return

    print(f"Procesando {len(archivos)} imágenes...")

    for nombre_archivo in archivos:
        ruta_img = os.path.join(CARPETA_ENTRADA, nombre_archivo)
        
        # Abrir imagen y asegurar que esté en modo RGB
        img = Image.open(ruta_img).convert('RGB')
        
        # Redimensionar si es necesario para que todos midan lo mismo
        img = img.resize((ANCHO_SPRITE, ALTO_SPRITE))
        
        datos_binarios = bytearray()

        # Recorrer cada píxel de la imagen
        for y in range(ALTO_SPRITE):
            for x in range(ANCHO_SPRITE):
                r, g, b = img.getpixel((x, y))
                
                # Convertir a 16 bits
                color_16 = convertir_a_rgb565(r, g, b)
                
                # Guardar como 2 bytes (Big Endian: Byte alto primero)
                # Esto es lo que la pantalla ILI9341 espera recibir
                datos_binarios.append((color_16 >> 8) & 0xFF)
                datos_binarios.append(color_16 & 0xFF)

        # Guardar el archivo .bin
        nombre_salida = os.path.splitext(nombre_archivo)[0] + ".bin"
        ruta_salida = os.path.join(CARPETA_SALIDA, nombre_salida)
        
        with open(ruta_salida, 'wb') as f:
            f.write(datos_binarios)
        
        print(f"Convertido: {nombre_archivo} -> {nombre_salida}")

    print("\n¡Proceso completado con éxito!")

if __name__ == "__main__":
    procesar_imagenes()