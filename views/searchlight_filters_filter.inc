<?php

class searchlight_filters_filter extends views_handler_filter {

  public $environment;

  /**
   * Overrides option_definition().
   */
  function option_definition() {
    $options = parent::option_definition();
    $options['facet_get'] = array('default' => array());
    $options['facet_set'] = array('default' => array());
    $options['expose']['contains']['facet_expose'] = array('default' => array());
    $options['expose']['contains']['facet_expose_get'] = array('default' => array());
    $options['expose']['contains']['facet_expose_set'] = array('default' => TRUE);
    return $options;
  }

  /**
   * Overrides admin_summary().
   */
  function admin_summary() {
    // TODO
  }

  /**
   * Overrides options_form().
   */
  function options_form(&$form, &$form_state) {
    $environment_name = $this->definition['environment_name'];
    $this->environment = searchlight_environment_load($environment_name);
    if (empty($this->environment)) {
      return;
    }

    $options = array();
    $this->environment->initView();
    foreach ($this->environment->plugins as $name => $plugin) {
      if (!empty($plugin->field)) {
        $options[$plugin->name] = $plugin->field['label'];
      }
    }
    $form['#facet_options'] = $options;

    $form['facet_get'] = array(
      '#type' => 'select',
      '#multiple' => TRUE,
      '#title' => t('Get facet results to current'),
      '#options' => $options,
      '#default_value' => $this->options['facet_get'],
    );
    // TODO: not in use.
//     $form['facet_set'] = array(
//       '#type' => 'select',
//       '#multiple' => TRUE,
//       '#title' => t('Set facet results to global'),
//       '#options' => $options,
//       '#default_value' => $this->options['facet_set'],
//     );
    parent::options_form($form, $form_state);
  }

  /**
   * Overrides pre_query().
   */
  function pre_query() {
    if (empty($this->environment)) {
      $environment_name = $this->definition['environment_name'];
      $this->environment = searchlight_environment_load($environment_name);
    }
    // So searchlight filter uses our environment.
    // TODO: another function?
    searchlight_environment_active($this->environment);
  }

  /**
   * Overrides query().
   */
  function query() {
    if (empty($this->environment)) {
      return;
    }

    // TODO: not in use.
//     if (!empty($this->options['facet_set'])) {
//     }

    $active_facets = searchlight_filters_active_facets();
    if (empty($active_facets)) {
      return;
    }

    // Filter the facets with options.
    // TODO: default to everything or only my own thing?
    if (!empty($this->options['facet_get'])) {
      $active_facets = array_intersect_key($active_facets, $this->options['facet_get']);
    }

    // Apply facet results.
    $this->environment->initValues($active_facets);

    // Use its own view to get facet results.
    // Force no pager and get all results.
    // TODO: this requires the dev version of Searchlight. Until it is merged...
    // $this->environment->views_options_override['pager'] = array('type' => 'none', 'options' => array('offset' => 0));
    $this->environment->initView();

    // Using the search result dircetly.
    if (!empty($this->environment->query->search_result)) {
      $query = &$this->environment->query;
      foreach ($this->environment->plugins as $name => $plugin) {
        if (array_key_exists($name, $active_facets) && !empty($plugin->field)) {
          if ($table_alias = $this->query->ensure_table($query->base_table, $this->relationship)) {
            $this->query->add_where($this->options['group'], "{$table_alias}.{$query->base_field}", $query->search_result['result'], 'IN');
          }
        }
      }
    }

    // Finished. Destroy.
    if (!empty($this->environment->view)) {
      $this->environment->view->destroy();
      unset($this->environment->view);
    }
  }

  /**
   * Overrides value_form().
   */
  function value_form(&$form, &$form_state) {
    // Default.
    $form['value'] = array(
      '#type' => 'value',
      '#value' => NULL,
    );

    if (empty($this->environment) || empty($this->options['expose']['facet_expose'])) {
      return;
    }

    // Add an identifier.
    $form['#searchlight_filters'] = !empty($form['#searchlight_filters']) ? $form['#searchlight_filters'] : array();

    // Apply facet results.
    if (!empty($this->options['expose']['facet_expose_get'])) {
      // Add an identifier.
      // TODO: not in use.
      $form['#searchlight_filters'][$this->options['expose']['identifier']] = $this->options['expose']['facet_expose_get'];

      $active_facets = searchlight_filters_active_facets();
      if (!empty($active_facets)) {
        $active_facets = array_intersect_key($active_facets, $this->options['expose']['facet_expose_get']);
        $this->environment->initValues($active_facets);
      }
    }

    // Use its own view to get facet results.
    $options = array();
    $this->environment->initView();
    if (!empty($this->environment->plugins) && array_key_exists($this->options['expose']['facet_expose'], $this->environment->plugins)) {
      $plugin = $this->environment->plugins[$this->options['expose']['facet_expose']];
      if ($render = $plugin->render(clone $this->environment->query, 'facets')) {
        foreach ($render as $item) {
          $options[$item['id']] = t('!title (!count)', array(
            '!title' => $item['title'],
            '!count' => $item['count'],
          ));
        }
      }
    }

    // Set from exposed input.
    // The input is from $this->view->get_exposed_input() originally.
    if (!empty($form_state['input']) && !empty($this->options['expose']['identifier'])) {
      $identifier = $this->options['expose']['identifier'];
      if (array_key_exists($identifier, $form_state['input'])) {
        // The exposed input is not valid.
        if (!array_key_exists($form_state['input'][$identifier], $options)) {
          unset($form_state['input'][$identifier]);
        }
        else {
          searchlight_filters_active_facets(array($this->options['expose']['facet_expose'] => $form_state['input'][$identifier]));
        }
      }
    }

    $form['value'] = array(
      '#type' => 'select',
      '#title' => empty($form_state['exposed']) ? t('Value') : '',
      '#options' => $options,
    );

    // Finished. Destroy.
    if (!empty($this->environment->view)) {
      $this->environment->view->destroy();
      unset($this->environment->view);
    }
  }

  /**
   * Overrides expose_form().
   */
  function expose_form(&$form, &$form_state) {
    if (!empty($form['#facet_options'])) {
      $form['expose']['facet_expose'] = array(
        '#type' => 'select',
        '#title' => t('Expose facet results'),
        '#options' => $form['#facet_options'],
        '#default_value' => $this->options['expose']['facet_expose'],
      );
      $form['expose']['facet_expose_get'] = array(
        '#type' => 'select',
        '#multiple' => TRUE,
        '#title' => t('Exposed form - Get facet results to current'),
        '#options' => $form['#facet_options'],
        '#default_value' => $this->options['expose']['facet_expose_get'],
      );
      $form['expose']['facet_expose_set'] = array(
        '#type' => 'checkbox',
        '#title' => t('Exposed form - Set facet results to global'),
        '#default_value' => $this->options['expose']['facet_expose_set'],
      );
    }
    parent::expose_form($form, $form_state);
  }
}
