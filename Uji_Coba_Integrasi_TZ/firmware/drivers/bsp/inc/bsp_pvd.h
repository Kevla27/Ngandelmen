/**
 * @file bsp_pvd.h
 * @brief Board Support Package untuk Deteksi Penurunan Catu Daya (PVD Last-Gasp)
 *        Memenuhi Standar SPLN D3.006: 2021 (Resiliensi Pemadaman / Data Retention)
 */

#ifndef BSP_PVD_H
#define BSP_PVD_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Tipe callback penanganan darurat saat catu daya anjlok melewati ambang batas PVD
 */
typedef void (*bsp_pvd_callback_t)(void);

/**
 * @brief Inisialisasi modul PVD pada level 2.8V - 2.9V dan aktifkan interupsi PVD/EXTI
 */
void bsp_pvd_init(void);

/**
 * @brief Mendaftarkan callback tambahan yang akan dieksekusi saat interupsi PVD terjadi
 * @param cb Pointer ke fungsi callback darurat
 */
void bsp_pvd_set_callback(bsp_pvd_callback_t cb);

/**
 * @brief Memeriksa apakah sistem saat ini berada dalam status darurat pemadaman daya
 * @return true jika tegangan anjlok (Power-Fail aktif), false jika normal
 */
bool bsp_pvd_is_power_failed(void);

/**
 * @brief Eksekusi urutan darurat Last-Gasp (Simpan Flash, matikan OLED/Buzzer, kunci memori)
 */
void bsp_pvd_execute_emergency_power_fail(void);

/**
 * @brief Simulasi penurunan daya darurat (untuk Unit Test CTest dan pengujian konsol CLI)
 */
void bsp_pvd_simulate_power_fail(void);

/**
 * @brief Simulasi pemulihan daya kembali normal
 */
void bsp_pvd_simulate_power_restore(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_PVD_H */

